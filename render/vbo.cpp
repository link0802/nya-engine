//https://code.google.com/p/nya-engine/

/*
    ToDo:
          draw arrays modes (quads, tris)
          is_valid function
          log
          gl functions in anonymous namespace
          advanced is_supported function (public)
*/

#include "vbo.h"
#include "platform_specific_gl.h"
#include "render.h"

namespace nya_render
{

#ifdef NO_EXTENSIONS_INIT
    #define vbo_glGenBuffers    glGenBuffers
    #define vbo_glBindBuffer    glBindBuffer
    #define vbo_glBufferData    glBufferData
    #define vbo_glDeleteBuffers    glDeleteBuffers
    #define vbo_glClientActiveTexture    glClientActiveTexture
#else
    PFNGLGENBUFFERSARBPROC vbo_glGenBuffers;
    PFNGLBINDBUFFERARBPROC vbo_glBindBuffer;
    PFNGLBUFFERDATAARBPROC vbo_glBufferData;
    PFNGLDELETEBUFFERSARBPROC vbo_glDeleteBuffers;
    PFNGLCLIENTACTIVETEXTUREARBPROC vbo_glClientActiveTexture;
#endif

bool check_init_vbo()
{
    static bool initialised = false;
    if(initialised)
        return true;

    //if(!has_extension("GL_ARB_vertex_buffer_object"))
    //    return false;

#ifndef NO_EXTENSIONS_INIT
    vbo_glGenBuffers = (PFNGLGENBUFFERSARBPROC) get_extension("glGenBuffers");
    if(!vbo_glGenBuffers)
        return false;

    vbo_glBindBuffer = (PFNGLBINDBUFFERARBPROC) get_extension("glBindBuffer");
    if(!vbo_glBindBuffer)
        return false;

    vbo_glBufferData = (PFNGLBUFFERDATAARBPROC)  get_extension("glBufferData");
    if(!vbo_glBufferData)
        return false;

    vbo_glDeleteBuffers = (PFNGLDELETEBUFFERSARBPROC)  get_extension("glDeleteBuffers");
    if(!vbo_glDeleteBuffers)
        return false;

    vbo_glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREARBPROC)  get_extension("glClientActiveTexture");
    if(!vbo_glClientActiveTexture)
        return false;
#endif

    initialised=true;
    return true;
}

void vbo::bind(bool indices_bind)
{
    bind_verts();
    bind_normals();
    bind_colors();

    for(int i=0;i<VBO_MAX_TEX_COORD;++i)
    {
        if(m_tcs[i].has)
            bind_tc(i);
    }
    if(indices_bind)
        bind_indices();
}

void vbo::bind_verts()
{
    if(!m_vertex_id)
        return;

    m_vertex_bind=true;

    glEnableClientState(GL_VERTEX_ARRAY);
    vbo_glBindBuffer(GL_ARRAY_BUFFER_ARB,m_vertex_id);

    const int element_dimensions=(m_element_type==quads)?4:3;

    glVertexPointer(element_dimensions,GL_FLOAT,m_vertex_stride,(void *)0);
}

void vbo::bind_normals()
{
    if(!m_vertex_bind)
        return;

    if(!m_normals.has)
        return;

    m_normals.bind=true;

    glNormalPointer(GL_FLOAT,m_vertex_stride,(void *)(m_normals.offset));
    glEnableClientState(GL_NORMAL_ARRAY);
}

void vbo::bind_colors()
{
    if(!m_vertex_bind)
        return;

    if(!m_colors.has)
        return;

    m_colors.bind=true;

    glColorPointer(m_colors.dimension,GL_FLOAT,m_vertex_stride,(void *)(m_colors.offset));
    glEnableClientState(GL_COLOR_ARRAY);
}

void vbo::bind_tc(unsigned int tc_idx)
{
    if(!m_vertex_bind)
        return;

    if(tc_idx>=VBO_MAX_TEX_COORD)
        return;

    attribute &tc=m_tcs[tc_idx];

    if(!tc.has)
        return;

    tc.bind=true;

    vbo_glClientActiveTexture(GL_TEXTURE0_ARB+tc_idx);
    glTexCoordPointer(tc.dimension,GL_FLOAT,m_vertex_stride,(void *)(tc.offset));
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void vbo::bind_indices()
{
    if(!m_index_id)
        return;

    m_index_bind=true;

    vbo_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB,m_index_id);
}

void vbo::unbind()
{
    if(m_vertex_bind)
    {
        glDisableClientState(GL_VERTEX_ARRAY);
        vbo_glBindBuffer(GL_ARRAY_BUFFER_ARB,0);
        m_vertex_bind=false;
    }

    if(m_index_bind)
    {
        vbo_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
        m_index_bind=false;
    }

    if(m_colors.bind)
    {
        glDisableClientState(GL_COLOR_ARRAY);
        m_colors.bind=false;
    }

    if(m_normals.bind)
    {
        glDisableClientState(GL_NORMAL_ARRAY);
        m_normals.bind=false;
    }

    bool has_unbinds=false;
    for(int i=0;i<VBO_MAX_TEX_COORD;++i)
    {
        attribute &tc=m_tcs[i];
        if(!tc.bind)
            continue;

        vbo_glClientActiveTexture(GL_TEXTURE0_ARB+i);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        tc.bind=false;
        has_unbinds=true;
    }
    if(has_unbinds)
        vbo_glClientActiveTexture(GL_TEXTURE0_ARB);
}

void vbo::draw()
{
    if(m_index_bind)
        draw(m_element_count);
    else
        draw(m_verts_count);
}

void vbo::draw(unsigned int count)
{
    draw(0,count);
}

void vbo::draw(unsigned int offset,unsigned int count)
{
    if(!m_vertex_bind)
        return;

    if(m_index_bind)
    {
        if(offset+count>m_element_count)
            return;

        const unsigned int gl_elem_type=(m_element_size==index4b?GL_UNSIGNED_INT:GL_UNSIGNED_SHORT);

        switch(m_element_type)
        {
            case triangles:
                glDrawElements(GL_TRIANGLES,count*3,gl_elem_type,(void*)(offset*3*m_element_size));
            break;

            case triangles_strip:
                glDrawElements(GL_TRIANGLE_STRIP,count*3,gl_elem_type,(void*)(offset*3*m_element_size));
            break;

            case triangles_fan:
                glDrawElements(GL_TRIANGLE_FAN,count*3,gl_elem_type,(void*)(offset*3*m_element_size));
            break;

            case quads:
                glDrawElements(GL_QUADS,count*4,gl_elem_type,(void*)(offset*4*m_element_size));
            break;
        }
    }
    else
    {
        if(offset+count>m_verts_count)
            return;

        glDrawArrays(GL_TRIANGLES,offset,count);
    }
}

void vbo::release()
{
    unbind();

    if(m_vertex_id)
        vbo_glDeleteBuffers(1,&m_vertex_id);

    if(m_index_id)
        vbo_glDeleteBuffers(1,&m_index_id);

    m_vertex_id=m_index_id=0;
}

void vbo::gen_vertex_data(const void*data,unsigned int vert_stride,unsigned int vert_count,bool dynamic)
{
    if(!check_init_vbo())
    {
        get_log()<<"Unable to gen vertices: vbo unsupported\n";
        return;
    }

    const unsigned int size=vert_count*vert_stride;
    if(size==0 || !data)
    {
        m_verts_count=0;
        get_log()<<"Unable to gen vertices: invalid data\n";
        return;
    }

    if(!m_vertex_id)
    {
        vbo_glGenBuffers(1,&m_vertex_id);
        vbo_glBindBuffer(GL_ARRAY_BUFFER_ARB,m_vertex_id);
    }

    if(dynamic)
        vbo_glBufferData(GL_ARRAY_BUFFER_ARB,size,data,GL_DYNAMIC_DRAW_ARB);
    else
        vbo_glBufferData(GL_ARRAY_BUFFER_ARB,size,data,GL_STATIC_DRAW_ARB);

    vbo_glBindBuffer(GL_ARRAY_BUFFER_ARB,0);

    m_vertex_stride=vert_stride;
    m_verts_count=vert_count;
}

void vbo::gen_index_data(const void*data,element_type type,element_size size,unsigned int faces_count,bool dynamic)
{
    if(!check_init_vbo())
    {
        get_log()<<"Unable to gen indexes: vbo unsupported\n";
        return;
    }

    const unsigned int buffer_size=faces_count*size*(type==quads?4:3);
    if(buffer_size==0 || !data)
    {
        get_log()<<"Unable to gen indexes: invalid data\n";
        m_element_count=0;
        return;
    }

    if(!m_index_id)
    {
        vbo_glGenBuffers(1,&m_index_id);
        vbo_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB,m_index_id);
    }

    m_element_count=faces_count;
    m_element_type=type;
    m_element_size=size;

    if(dynamic)
        vbo_glBufferData(GL_ELEMENT_ARRAY_BUFFER_ARB,buffer_size,data,GL_DYNAMIC_DRAW_ARB);
    else
        vbo_glBufferData(GL_ELEMENT_ARRAY_BUFFER_ARB,buffer_size,data,GL_STATIC_DRAW_ARB);

    vbo_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
}

void vbo::set_normals(unsigned int offset)
{
    m_normals.has=true;
    m_normals.offset=offset;
}

void vbo::set_tc(unsigned int tc_idx,unsigned int offset,unsigned int dimension)
{
    if(tc_idx>=VBO_MAX_TEX_COORD)
        return;

    attribute &tc=m_tcs[tc_idx];
    tc.has=true;
    tc.offset=offset;
    tc.dimension=dimension;
}

void vbo::set_colors(unsigned int offset,unsigned int dimension)
{
    m_colors.has=true;
    m_colors.offset=offset;
    m_colors.dimension=dimension;
}

}

