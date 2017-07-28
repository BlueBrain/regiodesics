#ifndef REGIODESICS_PROGRAMS_H
#define REGIODESICS_PROGRAMS_H

#include <osg/Program>
#include <osg/Shader>

inline void addShader(osg::Program* program, osg::Shader::Type type,
                      const char* source)
{
    osg::Shader* shader = new osg::Shader(type);
    shader->setShaderSource(source);
    program->addShader(shader);
}

inline osg::Program* createFlatShadingProgram()
{
    const char* vertexSource = R"(
    #version 120
    void main()
    {
        gl_FrontColor = gl_Color;
        gl_Position = gl_ModelViewMatrix * gl_Vertex;
    })";

    const char* geomSource = R"(
    #version 120
    #extension GL_EXT_geometry_shader4 : enable
    #extension GL_EXT_gpu_shader4 : enable

    varying out vec3 normal;
    varying out vec3 eye;

    void main()
    {
        normal = cross(gl_PositionIn[1].xyz - gl_PositionIn[0].xyz,
                       gl_PositionIn[2].xyz - gl_PositionIn[0].xyz);
        if (normal.z < 0)
            normal = -normal;

        gl_FrontColor = gl_FrontColorIn[0];
        for (int i = 0; i < 3; ++i)
        {
            gl_Position = gl_ProjectionMatrix * gl_PositionIn[i];
            eye = -gl_PositionIn[i].xyz;
            EmitVertex();
        }
        EndPrimitive();
    })";

    const char* const fragSource = R"(
    #version 120
    #extension GL_EXT_geometry_shader4 : enable

    varying vec3 normal;
    varying vec3 eye;

    void main()
    {
        vec3 norm_eye = normalize(eye);
        float lambert = normalize(normal).z * 0.9;
        vec4 color = gl_Color * lambert + vec4(0.05, 0.05, 0.05, 0.0);

        vec3 r = reflect(-norm_eye, normal);
        color.rgb += vec3(0.05, 0.05, 0.05) *
                     pow(max(dot(r, norm_eye) , 0.0), 16.0);

        gl_FragColor = color;
    })";

    osg::Program* program = new osg::Program();
    addShader(program, osg::Shader::VERTEX, vertexSource);
    addShader(program, osg::Shader::GEOMETRY, geomSource);
    addShader(program, osg::Shader::FRAGMENT, fragSource);
    program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 3);
    program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
    program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);

    return program;
}

inline osg::Program* createLitLinesProgram()
{
    const char* vertexSource = R"(
    #version 120
    void main()
    {
        gl_FrontColor = gl_Color;
        gl_Position = gl_ModelViewMatrix * gl_Vertex;
    })";

    const char* geomSource = R"(
    #extension GL_EXT_geometry_shader4 : enable
    #extension GL_EXT_gpu_shader4 : enable

    varying out vec3 normal;
    varying out vec3 eye;
    varying out float t;

    void main()
    {
        vec3 tangent = normalize(gl_PositionIn[1].xyz - gl_PositionIn[0].xyz);

        for (int i = 0; i < 2; ++i)
        {
            gl_FrontColor = gl_FrontColorIn[i];
            gl_Position = gl_ProjectionMatrix * gl_PositionIn[i];
            t = i;
            eye = -gl_PositionIn[i].xyz;
            vec3 v = normalize(cross(eye, tangent));
            normal = cross(tangent, v);
            if (normal.z < 0)
                normal = -normal;
            EmitVertex();
        }
        EndPrimitive();
    })";

    const char* const fragSource = R"(
    #version 120
    #extension GL_EXT_geometry_shader4 : enable

    varying vec3 normal;
    varying vec3 eye;
    varying float t;

    void main()
    {
        vec3 norm_eye = normalize(eye);
        float lambert = normalize(normal).z * 0.8;
        vec4 color = gl_Color * lambert + vec4(0.05, 0.05, 0.05, 0.0);

        vec3 r = reflect(-norm_eye, normal);
        color.rgb += vec3(0.15, 0.15, 0.15) *
                     pow(max(dot(r, norm_eye) , 0.0), 16.0);

        gl_FragColor = color;
    })";

    osg::Program* program = new osg::Program();
    addShader(program, osg::Shader::VERTEX, vertexSource);
    addShader(program, osg::Shader::GEOMETRY, geomSource);
    addShader(program, osg::Shader::FRAGMENT, fragSource);
    program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 2);
    program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_LINES);
    program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_LINE_STRIP);

    return program;
}

#endif
