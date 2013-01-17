//https://code.google.com/p/nya-engine/

#pragma once

#include "math/vector.h"
#include "math/quaternion.h"
#include "math/bezier.h"
#include <vector>
#include <map>
#include <string>

namespace nya_render
{

class animation
{
public:
    struct bone
    {
        nya_math::vec3 pos;
        nya_math::quat rot;
    };

public:
    //unsigned int get_frames_count() const; //?
    unsigned int get_duration() const { return m_duration; }
    int get_bone_idx(const char *name) const; //< 0 if invalid
    bone get_bone(int idx,unsigned int time,bool looped=true) const;

public:
    int add_bone(const char *name); //create or return existing
    void add_bone_frame(int idx,unsigned int time,bone &b);

    struct interpolation
    {
        nya_math::bezier pos_x;
        nya_math::bezier pos_y;
        nya_math::bezier pos_z;
        nya_math::bezier rot;
    };

    void add_bone_frame(int idx,unsigned int time,bone &b,interpolation &i);

public:
    animation(): m_duration(0) {}

private:
    struct frame
    {
        unsigned int time;
        bone pos_rot;
        interpolation inter;

        frame(): time(0) {}
    };

    struct sequence
    {
        std::vector<frame> frames;
    };

    typedef std::map<std::string,unsigned int> index_map;
    index_map m_bones_map;
    std::vector<sequence> m_bones;

    unsigned int m_duration;
};

}
