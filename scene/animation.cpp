//https://code.google.com/p/nya-engine/

#include "animation.h"
#include "memory/memory_reader.h"

namespace nya_scene
{

bool animation::load(const char *name)
{
    if(!scene_shared::load(name))
        return false;

    if(!m_shared.is_valid())
        return false;

    m_range_from=0;
    m_range_to=m_shared->anim.get_duration();

    m_speed=m_weight=1.0f;

    update_version();

    if(m_mask)
    {
        delete m_mask;
        m_mask=0;
    }

    return true;
}

void animation::unload()
{
    scene_shared::unload();

    m_range_from=m_range_to=0;
    m_speed=m_weight=1.0f;

    if(m_mask)
    {
        delete m_mask;
        m_mask=0;
    }
}

bool animation::load_vmd(shared_animation &res,resource_data &data,const char* name)
{
    nya_memory::memory_reader reader(data.get_data(),data.get_size());

    if(!reader.test("Vocaloid Motion Data 0002",25))
        return false;

    reader.skip(5);
    reader.skip(20);//name

    typedef unsigned int uint;

    const uint frames_count=reader.read<uint>();
    if(!reader.check_remained(frames_count*(15+sizeof(uint)+sizeof(float)*7+16*4)))
        return false;

    for(uint i=0;i<frames_count;++i)
    {
        struct
        {
            std::string name;
            uint frame;

            float pos[3];
            float rot[4];

            char bezier_x[16];
            char bezier_y[16];
            char bezier_z[16];
            char bezier_rot[16];

        } bone_frame;

        bone_frame.name=std::string((const char*)reader.get_data(),15);
        reader.skip(15);
        bone_frame.frame=reader.read<uint>();
        bone_frame.pos[0]=reader.read<float>();
        bone_frame.pos[1]=reader.read<float>();
        bone_frame.pos[2]=-reader.read<float>();

        bone_frame.rot[0]=-reader.read<float>();
        bone_frame.rot[1]=-reader.read<float>();
        bone_frame.rot[2]=reader.read<float>();
        bone_frame.rot[3]=reader.read<float>();

        memcpy(bone_frame.bezier_x,reader.get_data(),sizeof(bone_frame.bezier_x));
        reader.skip(sizeof(bone_frame.bezier_x));
        memcpy(bone_frame.bezier_y,reader.get_data(),sizeof(bone_frame.bezier_y));
        reader.skip(sizeof(bone_frame.bezier_y));
        memcpy(bone_frame.bezier_z,reader.get_data(),sizeof(bone_frame.bezier_z));
        reader.skip(sizeof(bone_frame.bezier_z));
        memcpy(bone_frame.bezier_rot,reader.get_data(),sizeof(bone_frame.bezier_rot));
        reader.skip(sizeof(bone_frame.bezier_rot));

        const float c2f=1.0f/128.0f;

        const int bone_idx=res.anim.add_bone(bone_frame.name.c_str());
        if(bone_idx<0)
            continue;

        const uint time=bone_frame.frame*60; //ToDo: adjust framerate

        nya_render::animation::bone bone;
        nya_render::animation::interpolation interpolation;

        bone.pos=nya_math::vec3(bone_frame.pos);
        bone.rot=nya_math::quat(bone_frame.rot);

        interpolation.pos_x=nya_math::bezier(bone_frame.bezier_x[0]*c2f,bone_frame.bezier_x[4]*c2f,
                                             bone_frame.bezier_x[8]*c2f,bone_frame.bezier_x[12]*c2f);

        interpolation.pos_y=nya_math::bezier(bone_frame.bezier_y[0]*c2f,bone_frame.bezier_y[4]*c2f,
                                             bone_frame.bezier_y[8]*c2f,bone_frame.bezier_y[12]*c2f);

        interpolation.pos_z=nya_math::bezier(bone_frame.bezier_z[0]*c2f,bone_frame.bezier_z[4]*c2f,
                                             bone_frame.bezier_z[8]*c2f,bone_frame.bezier_z[12]*c2f);

        interpolation.rot=nya_math::bezier(bone_frame.bezier_rot[0]*c2f,bone_frame.bezier_rot[4]*c2f,
                                           bone_frame.bezier_rot[8]*c2f,bone_frame.bezier_rot[12]*c2f);

        res.anim.add_bone_frame(bone_idx,time,bone,interpolation);
    }

    return true;
}

unsigned int animation::get_duration() const
{
    if(!m_shared.is_valid())
        return 0;

    return m_shared->anim.get_duration();
}

void animation::set_range(unsigned int from,unsigned int to)
{
    if(!m_shared.is_valid())
        return;

    m_range_from=from;
    m_range_to=to;

    unsigned int duration = m_shared->anim.get_duration();
    if(m_range_from>duration)
        m_range_from=duration;

    if(m_range_to>duration)
        m_range_to=duration;

    if(m_range_from>m_range_to)
        m_range_from=m_range_to;
}

void animation::mask_all(bool enabled)
{
    if(enabled)
    {
        if(m_mask)
        {
            delete m_mask;
            m_mask=0;
            update_version();
        }
    }
    else
    {
        if(!m_mask)
            m_mask=new mask_data;
        else
            m_mask->data.clear();

        update_version();
    }
}

void animation::update_version()
{
    static unsigned int version = 0;
    m_version= ++version;
}

void animation::add_mask(const char *name,bool enabled)
{
    if(!name)
        return;

    if(!m_shared.is_valid())
        return;

    if(m_shared->anim.get_bone_idx(name)<0)
        return;

    if(enabled)
    {
        if(!m_mask)
            return;

        m_mask->data[name]=true;
        update_version();
    }
    else
    {
        if(!m_mask)
        {
            m_mask=new mask_data;
            for(int i=0;i<m_shared->anim.get_bones_count();++i)
                m_mask->data[m_shared->anim.get_bone_name(i)]=true;
        }

        std::map<std::string,bool>::iterator it=m_mask->data.find(name);
        if(it!=m_mask->data.end())
            m_mask->data.erase(it);

        update_version();
    }
}

}
