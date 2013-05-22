//https://code.google.com/p/nya-engine/

#pragma once

#include "shader.h"
#include "texture.h"
#include "proxy.h"
#include "render/render.h"
#include <string>
#include <vector>

namespace nya_scene
{

typedef proxy<texture> texture_proxy;

class material
{
    friend class mesh;

public:
    class param
    {
        friend class material;

    public:
        void set(float f0,float f1,float f2,float f3)
        {
            f[0]=f0; f[1]=f1;
            f[2]=f2; f[3]=f3;
        }

        param() { for(int i=0;i<4;++i) f[i]=0.0f; }
        param(float f0,float f1,float f2,float f3) { set(f0,f1,f2,f3); }

    private:
        float f[4];
    };
    
    typedef proxy<param> param_proxy;

    class param_array
    {
        friend class material;

    public:
        void set_count(int count) { m_params.resize(count); }
        int get_count() const { return (int)m_params.size(); }
        void set(int idx,const param &p) { if(idx>=0 && idx<(int)m_params.size()) m_params[idx]=p; }
        void set(int idx,float f0,float f1,float f2,float f3)
        {
            if(idx>=0 && idx<(int)m_params.size())
                m_params[idx].set(f0,f1,f2,f3);
        }

    private:
        std::vector<param> m_params;
    };

    typedef proxy<param_array> param_array_proxy;

public:
    typedef nya_render::blend::mode blend_mode;
    typedef nya_render::cull_face::order cull_order;

public:
    const char *get_name() const { return m_name.c_str(); }

    int get_textures_count() const { return (int)m_textures.size(); }
    const char *get_texture_name(int idx) const;
    const char *get_texture_semantics(int idx) const;

    int get_params_count() const { return m_shader.get_uniforms_count(); }
    const char *get_param_name(int idx) const;
    int get_param_idx(const char *name) const;
    void set_param(int idx,float f0,float f1,float f2,float f3);
    void set_param(int idx,const param &p);
    void set_param(int idx,const param_proxy &p);
    void set_param(int idx,const param_proxy &p,const param &m); //set as p*m
    void set_param(int idx,const param_proxy &p,const param_proxy &m);
    void set_param_array(int idx,const param_array & a);
    void set_param_array(int idx,const param_array_proxy & p);

    const param_proxy &get_param(int idx);
    const param_proxy &get_param_multiplier(int idx);
    const param_array_proxy &get_param_array(int idx);

    bool get_zwrite() const { return m_zwrite; }
    bool get_color_write() const { return m_color_write; }
    bool get_blend(blend_mode &src,blend_mode &dst) const { 
                   src=m_blend_src; dst=m_blend_dst; return m_blend; }
    bool get_blend() const { return m_blend; }

public:
    void release();

public:
    void set_name(const char*name) { m_name.assign(name?name:""); }
    void set_texture(const char *semantics,const texture &tex);
    void set_texture(const char *semantics,const texture_proxy &proxy);
    void set_shader(const shader &shdr);
    void set_zwrite(bool enabled) { m_zwrite=enabled; }
    void set_color_write(bool enabled) { m_color_write=enabled; }
    void set_blend(bool enabled,blend_mode src,blend_mode dst);
    void set_blend(bool enabled) { m_blend=enabled; }
    void set_cull_face(bool enabled,cull_order order) { m_cull_face=enabled; m_cull_order=order; }
    void set_cull_face(bool enabled) { m_cull_face=enabled; }

private:
    void set() const;
    void unset() const;

public:
    material(): m_zwrite(true),m_color_write(true),m_blend(false),m_cull_face(false),m_cull_order(nya_render::cull_face::ccw),
                m_blend_src(nya_render::blend::one),m_blend_dst(nya_render::blend::zero) {}

private:
    std::string m_name;

    shader m_shader;

    bool m_zwrite;
    bool m_color_write;
    bool m_blend;
    bool m_cull_face;
    cull_order m_cull_order;
    blend_mode m_blend_src;
    blend_mode m_blend_dst;

    struct material_texture
    {
        std::string semantics;
        int slot;
        texture_proxy proxy;
    };

    std::vector<material_texture> m_textures;

    struct param_holder
    {
        param_proxy p;
        param_proxy m;
        param_array_proxy a;
    };

    std::vector<param_holder> m_params;
};

}
