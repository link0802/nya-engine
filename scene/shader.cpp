//https://code.google.com/p/nya-engine/

#include "shader.h"
#include "scene.h"
#include "camera.h"
#include "transform.h"
#include "render/render.h"
#include <string.h>
#include <cstdlib>

namespace nya_scene
{

struct shader_description
{
    struct predefined
    {
        std::string name;
        shared_shader::transform_type transform;
    };

    predefined predefines[shared_shader::predefines_count];

    typedef std::map<std::string,std::string> strings_map;
    strings_map samplers;
    strings_map uniforms;
};

bool load_nya_shader_internal(shared_shader &res,shader_description &desc,resource_data &data,const char* name,bool include)
{
    size_t data_size=data.get_size();
    if(!data_size)
        return false;

    const char *text=(const char*)data.get_data();

    for(size_t i=0;i<data_size-1;++i)
    {
        if(text[i]!='@')
            continue;

        ++i;

        char check=text[i];
        switch (check)
        {
            case 'i':
                if(i+7<data_size && strncmp(&text[i],"include",7)==0)
                {
                    i+=7;
                    for(size_t begin=std::string::npos;i<data_size;++i)
                    {
                        if(text[i]=='"')
                        {
                            if(begin==std::string::npos)
                            {
                                begin=i+1;
                                continue;
                            }

                            std::string path(name);
                            size_t p=path.rfind("/");
                            if(p==std::string::npos)
                                p=path.rfind("\\");

                            if(p==std::string::npos)
                                path.clear();
                            else
                                path.resize(p+1);

                            path.append(&text[begin],i-begin);

                            nya_resources::resource_data *file_data=nya_resources::get_resources_provider().access(path.c_str());
                            if(!file_data)
                            {
                                get_log()<<"unable to load shader include resource: unable to access resource "<<path.c_str()<<"\n";
                                return false;
                            }

                            const size_t data_size=file_data->get_size();
                            nya_memory::tmp_buffer_ref include_data(data_size);
                            file_data->read_all(include_data.get_data());
                            file_data->release();

                            if(!load_nya_shader_internal(res,desc,include_data,path.c_str(),true))
                            {
                                get_log()<<"unable to load shader include: unknown format in "<<path.c_str()<<"\n";
                                include_data.free();
                                return false;
                            }

                            include_data.free();
                        }

                        if(text[i]=='\n' || text[i]=='\r')
                            break;
                    }

                    --i;
                }
                break;

            case 'a':
                if(i+3<data_size && strncmp(&text[i],"all",3)==0)
                {
                    size_t begin=i+3;
                    for(i=begin;i<data_size;++i)
                        if(text[i]=='@')
                            break;

                    res.vertex.append(&text[begin],i-begin);
                    res.pixel.append(&text[begin],i-begin);
                    --i;
                }
                break;

            case 's':
                if(i+7<data_size && strncmp(&text[i],"sampler",7)==0)
                {
                    i+=7;
                    while(i<data_size && (text[i]==' ' || text[i]=='\t')) ++i;

                    size_t begin=i;
                    while(i<data_size && text[i]!=' ' && text[i]!='\t' && text[i]!='"') ++i;

                    std::string sampler_name(&text[begin],i-begin);

                    for(begin=std::string::npos;i<data_size;++i)
                    {
                        if(text[i]=='"')
                        {
                            if(begin==std::string::npos)
                            {
                                begin=i+1;
                                continue;
                            }

                            std::string sampler_semantics=std::string(&text[begin],i-begin);
                            desc.samplers[sampler_semantics]=sampler_name;
                            res.samplers[sampler_semantics]=res.samplers_count;
                            ++res.samplers_count;
                        }

                        if(text[i]=='\n' || text[i]=='\r')
                            break;
                    }
                }
                break;

            case 'v':
                if(i+6<data_size && strncmp(&text[i],"vertex",6)==0)
                {
                    size_t begin=i+6;
                    for(i=begin;i<data_size;++i)
                        if(text[i]=='@')
                            break;

                    res.vertex.append(&text[begin],i-begin);
                    --i;
                }
            break;

            case 'f':
                if(i+8<data_size && strncmp(&text[i],"fragment",8)==0)
                {
                    size_t begin=i+8;
                    for(i=begin;i<data_size;++i)
                        if(text[i]=='@')
                            break;

                    res.pixel.append(&text[begin],i-begin);
                    --i;
                }
            break;

            case 'p':
            case 'u':
                if((check=='p' && i+10<data_size && strncmp(&text[i],"predefined",10)==0)
                   || (check=='u' && i+7<data_size && strncmp(&text[i],"uniform",7)==0))
                {
                    i+=(check=='p')?10:7;

                    while(i<data_size && (text[i]==' ' || text[i]=='\t')) ++i;

                    size_t begin=i;
                    while(i<data_size && text[i]!=' ' && text[i]!='\t' && text[i]!='"') ++i;

                    std::string name(&text[begin],i-begin);

                    for(begin=std::string::npos;i<data_size;++i)
                    {
                        if(text[i]=='"')
                        {
                            if(begin==std::string::npos)
                            {
                                begin=i+1;
                                continue;
                            }

                            std::string semantics=std::string(&text[begin],i-begin);

                            shared_shader::transform_type transform=shared_shader::none;
                            float default_value[4]={0.0,0.0,0.0,0.0};
                            for(begin=std::string::npos;i<data_size;++i)
                            {
                                if(text[i]=='\n' || text[i]=='\r')
                                {
                                    if(begin!=std::string::npos)
                                    {
                                        std::string params=std::string(&text[begin],i-begin);
                                        if(params.find("local_rot")!=std::string::npos)
                                            transform=shared_shader::local_rot;
                                        else if(params.find("local")!=std::string::npos)
                                            transform=shared_shader::local;
                                    }

                                    break;
                                }

                                if(text[i]==':')
                                    begin=i+1;
                                else if(text[i]=='=')
                                {
                                    ++i;
                                    for(int j=0;j<4;++j)
                                    {
                                        while(i<data_size && (text[i]==' ' || text[i]=='\t' || text[i]==',')) ++i;
                                        if(text[i]=='\n' || text[i]=='\r' || text[i]==':' ) { --i; break; }
                                        default_value[j]=(float)atof(&text[i]);
                                        while(i<data_size && ((text[i]>= '0' && text[i]<='9') || text[i]=='.' || text[i]=='-')) ++i;
                                        if(text[i]=='\n' || text[i]=='\r' || text[i]==':' ) { --i; break; }
                                    }

                                    default_value[0]=default_value[0];
                                }
                            }

                            if(check=='p')
                            {
                                const char *predefined_semantics[]={"nya camera position","nya camera rotation","nya bones pos","nya bones rot","nya viewport",
                                                                    "nya model pos","nya model rot","nya model scale"};

                                char predefined_count_static_assert[sizeof(predefined_semantics)/sizeof(predefined_semantics[0])
                                                                                        ==shared_shader::predefines_count?1:-1];
                                predefined_count_static_assert[0]=0;
                                for(int i=0;i<shared_shader::predefines_count;++i)
                                {
                                    if(semantics==predefined_semantics[i])
                                    {
                                        desc.predefines[i].name=name;
                                        desc.predefines[i].transform=transform;
                                        break;
                                    }
                                }
                            }
                            else if(desc.uniforms.find(semantics)==desc.uniforms.end())
                            {
                                desc.uniforms[semantics]=name;
                                res.uniforms.resize(res.uniforms.size()+1);
                                res.uniforms.back().name=semantics;
                                res.uniforms.back().transform=transform;
                                res.uniforms.back().default_value=default_value;
                            }
                        }

                        if(text[i]=='\n' || text[i]=='\r')
                            break;
                    }
                }
                break;

            default:
                get_log()<<"scene shader load warning: unsupported shader tag in "<<name<<"\n";
            break;
        }
    }

    if(include)
    {
        data.free();
        return true;
    }

    if(res.vertex.empty())
    {
        get_log()<<"scene shader load error: empty vertex shader in "<<name<<"\n";
        return false;
    }

    if(res.pixel.empty())
    {
        get_log()<<"scene shader load error: empty pixel shader in "<<name<<"\n";
        return false;
    }

    //get_log()<<"vertex <"<<res.vertex.c_str()<<">\n";
    //get_log()<<"pixel <"<<res.pixel.c_str()<<">\n";

    for(shared_shader::samplers_map::iterator it=res.samplers.begin();
        it!=res.samplers.end();++it)
        res.shdr.set_sampler(desc.samplers[it->first].c_str(),it->second);

    if(!res.shdr.add_program(nya_render::shader::vertex,res.vertex.c_str()))
        return false;

    if(!res.shdr.add_program(nya_render::shader::pixel,res.pixel.c_str()))
        return false;

    for(int i=0;i<shared_shader::predefines_count;++i)
    {
        const shader_description::predefined &p=desc.predefines[i];
        if(p.name.empty())
            continue;

        res.predefines.resize(res.predefines.size()+1);
        res.predefines.back().transform=p.transform;
        res.predefines.back().type=(shared_shader::predefined_values)i;
        res.predefines.back().location=res.shdr.get_handler(p.name.c_str());
    }

    for(int i=0;i<(int)res.uniforms.size();++i)
        res.uniforms[i].location=res.shdr.get_handler(desc.uniforms[res.uniforms[i].name].c_str());

    data.free();

    return true;
}

bool shader::load_nya_shader(shared_shader &res,resource_data &data,const char* name)
{
    shader_description desc;
    return load_nya_shader_internal(res,desc,data,name,false);
}

void shader_internal::set() const
{
    if(!m_shared.is_valid())
        return;

    m_shared->shdr.bind();

    for(size_t i=0;i<m_shared->predefines.size();++i)
    {
        const shared_shader::predefined &p=m_shared->predefines[i];
        switch(p.type)
        {
            case shared_shader::camera_pos:
            {
                if(!get_camera().is_valid())
                {
                    m_shared->shdr.set_uniform(p.location,0.0f,0.0f,0.0f);
                    break;
                }

                if(p.transform==shared_shader::local)
                {
                    const nya_math::vec3 v=transform::get().inverse_transform(get_camera()->get_pos());
                    m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
                }
                else if(p.transform==shared_shader::local_rot)
                {
                    const nya_math::vec3 v=transform::get().inverse_rot(get_camera()->get_pos());
                    m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
                }
                else
                {
                    const nya_math::vec3 v=get_camera()->get_pos();
                    m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
                }
            }
            break;

            case shared_shader::camera_rot:
            {
                if(!get_camera().is_valid())
                {
                    m_shared->shdr.set_uniform(p.location,0.0f,0.0f,0.0f);
                    break;
                }

                //if(p.transform==shared_shader::none) //ToDo
                {
                    const nya_math::quat v=get_camera()->get_rot();
                    m_shared->shdr.set_uniform(p.location,v.v.x,v.v.y,v.v.z,v.w);
                }
            }

            case shared_shader::bones_pos:
            {
                if(m_skeleton && m_shared->last_skeleton_pos!=m_skeleton)
                {
                    m_shared->shdr.set_uniform3_array(p.location,m_skeleton->get_pos_buffer(),m_skeleton->get_bones_count());
                    m_shared->last_skeleton_pos=m_skeleton;
                }
            }
            break;

            case shared_shader::bones_rot:
            {
                if(m_skeleton && m_shared->last_skeleton_rot!=m_skeleton)
                {
                    m_shared->shdr.set_uniform4_array(p.location,m_skeleton->get_rot_buffer(),m_skeleton->get_bones_count());
                    m_shared->last_skeleton_rot=m_skeleton;
                }
            }
            break;

            case shared_shader::viewport:
            {
                nya_render::rect r=nya_render::get_viewport();
                m_shared->shdr.set_uniform(p.location,float(r.x),float(r.y),float(r.width),float(r.height));
            }
            break;

            case shared_shader::model_pos:
            {
                const nya_math::vec3 v=transform::get().get_pos();
                m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
            }
            break;

            case shared_shader::model_rot:
            {
                const nya_math::quat v=transform::get().get_rot();
                m_shared->shdr.set_uniform(p.location,v.v.x,v.v.y,v.v.z,v.w);
            }
            break;

            case shared_shader::model_scale:
            {
                const nya_math::vec3 v=transform::get().get_scale();
                m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
            }
            break;

            case shared_shader::predefines_count: break;
        }
    }
}

void shader_internal::unset() const
{
    if(!m_shared.is_valid())
        return;

    m_shared->shdr.unbind();
}

int shader_internal::get_texture_slot(const char *semantics) const
{
    if(!semantics || !m_shared.is_valid())
        return -1;

    shared_shader::samplers_map::const_iterator it=m_shared->samplers.find(semantics);
    if(it==m_shared->samplers.end())
        return -1;

    return it->second;
}

int shader_internal::get_texture_slots_count() const
{
    if(!m_shared.is_valid())
        return 0;

    return m_shared->samplers_count;
}

const shared_shader::uniform &shader_internal::get_uniform(int idx) const
{
    if(!m_shared.is_valid() || idx<0 || idx >=(int)m_shared->uniforms.size())
    {
        static shared_shader::uniform invalid;
        return invalid;
    }

    return m_shared->uniforms[idx];
}

void shader_internal::set_uniform_value(int idx,float f0,float f1,float f2,float f3) const
{
    if(!m_shared.is_valid() || idx<0 || idx >=(int)m_shared->uniforms.size())
        return;

    if(m_shared->uniforms[idx].location<0)
        return;

    if(m_shared->uniforms[idx].transform==shared_shader::local)
    {
        nya_math::vec3 v=transform::get().inverse_transform(nya_math::vec3(f0,f1,f2));
        m_shared->shdr.set_uniform(m_shared->uniforms[idx].location,v.x,v.y,v.z,f3);
    }
    else if(m_shared->uniforms[idx].transform==shared_shader::local_rot)
    {
        nya_math::vec3 v=transform::get().inverse_rot(nya_math::vec3(f0,f1,f2));
        m_shared->shdr.set_uniform(m_shared->uniforms[idx].location,v.x,v.y,v.z,f3);
    }
    else
        m_shared->shdr.set_uniform(m_shared->uniforms[idx].location,f0,f1,f2,f3);
}

void shader_internal::set_uniform4_array(int idx,const float *array,int size) const
{
    if(!m_shared.is_valid() || idx<0 || idx >=(int)m_shared->uniforms.size())
        return;

    m_shared->shdr.set_uniform4_array(m_shared->uniforms[idx].location,array,size);
}

int shader_internal::get_uniforms_count() const
{
    if(!m_shared.is_valid())
        return 0;

    return (int)m_shared->uniforms.size();
}

const nya_render::skeleton *shader_internal::m_skeleton=0;

}
