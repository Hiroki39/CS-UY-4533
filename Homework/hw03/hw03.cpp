/*
    hw02.cpp
    CS-UY 4533, Nov 3rd, 2020
    Hongyi Zheng
    Rolling Sphere
*/
#include <fstream>
#include <vector>
#include "Angel-yjc.hpp"
using namespace std;

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

GLuint program;        // shader program object id
GLuint sphere_buffer;  // vertex buffer object id for sphere
GLuint floor_buffer;   // vertex buffer object id for floor
GLuint axis_buffer;    // vertex buffer object id for axis
GLuint shadow_buffer;  // vertex buffer object id for shadow

// Projection transformation parameters
GLfloat fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat aspect;       // Viewport aspect ratio
GLfloat zNear = 0.5, zFar = 20.0;

GLfloat angle = 0.0;                  // rotation angle in degrees
vec4 init_eye(7.0, 3.0, -10.0, 1.0);  // initial viewer position
vec4 eye = init_eye;                  // current viewer position

// 1: animation; 0: non-animation. Toggled by key 'a' or 'A'
bool animation = false;
// 1: solid sphere; 0: wireframe sphere. Toggled by key 'c' or 'C'
bool solidSphere = true;
// 1: solid floor; 0: wireframe floor. Toggled by key 'f' or 'F'
bool solidFloor = true;

bool shadow = true;

bool smoothShading = false;
bool spotlight = false;
bool lighting = true;
bool b_key_pressed = false;

vector<vec4> sphere_points;   // positions for all sphere vertices
vector<vec4> sphere_colors;   // colors for all sphere vertices
vector<vec3> sphere_normals;  // normals for all sphere vertices

vector<vec4> floor_points;   // positions for all floor vertices
vector<vec4> floor_colors;   // colors for all floor vertices
vector<vec3> floor_normals;  // normals for all floor vertices

vector<vec4> axis_points;  // positions for all axis vertices
vector<vec4> axis_colors;  // colors for all axis vertices

vector<vec4> shadow_colors;  // positions for all shadow vertices

vec4 material_ambient;
vec4 material_diffuse;
vec4 material_specular;
// In World frame.
// Needs to transform it to Eye Frame
// before sending it to the shader(s).
vec4 light_position(-14.0, 12.0, -3.0, 1.0);
vec4 spotlight_destination(-6.0, 0.0, -4.5, 1.0);

mat4 accumulated_rotation(1.0);
vector<vec4> rolling_points{vec4(-4, 1, 4, 1), vec4(3, 1, -4, 1),
                            vec4(-3, 1, -3, 1)};
int num_pts = rolling_points.size();
int curr_pt_index = 0;

// generate 2 triangles: 6 vertices
void floor() {
    floor_points.emplace_back(5, 0, 8, 1);
    floor_points.emplace_back(5, 0, -4, 1);
    floor_points.emplace_back(-5, 0, -4, 1);
    floor_points.emplace_back(-5, 0, -4, 1);
    floor_points.emplace_back(-5, 0, 8, 1);
    floor_points.emplace_back(5, 0, 8, 1);
    for (int i = 0; i < floor_points.size(); ++i) {
        floor_colors.emplace_back(0, 1, 0, 1);
        floor_normals.emplace_back(0, 1, 0);
    }
}

// generate 3 axis: 6 vertices
void axis() {
    // x-axis
    axis_points.emplace_back(0, 0, 0, 1);
    axis_colors.emplace_back(1, 0, 0, 1);
    axis_points.emplace_back(10, 0, 0, 1);
    axis_colors.emplace_back(1, 0, 0, 1);

    // y-axis
    axis_points.emplace_back(0, 0, 0, 1);
    axis_colors.emplace_back(1, 0, 1, 1);
    axis_points.emplace_back(0, 10, 0, 1);
    axis_colors.emplace_back(1, 0, 1, 1);

    // z-axis
    axis_points.emplace_back(0, 0, 0, 1);
    axis_colors.emplace_back(0, 0, 1, 1);
    axis_points.emplace_back(0, 0, 10, 1);
    axis_colors.emplace_back(0, 0, 1, 1);
}

void read_file() {
    ifstream sphereStream;  // create a filestream
    sphereStream.clear();
    char selection;
    string filename;

    // prompt user to select a file
    cout << "Select Number of Vertices of the Sphere" << endl;
    cout << "a) 8" << endl;
    cout << "b) 128" << endl;
    cout << "c) 256" << endl;
    cout << "d) 1024" << endl;
    cout << "Enter your choice: ";
    cin >> selection;
    switch (selection) {
        case 'a':
        case 'A':
            sphereStream.open("sphere.8");
            break;
        case 'b':
        case 'B':
            sphereStream.open("sphere.128");
            break;
        case 'c':
        case 'C':
            sphereStream.open("sphere.256");
            break;
        case 'd':
        case 'D':
            sphereStream.open("sphere.1024");
            break;
        default:
            cout << "Invalid Option" << endl;
            exit(1);
            break;
    }

    // check if the file is succesfully opened
    if (!sphereStream) {
        cerr << "failed to open " << filename << endl;
        exit(1);
    }

    int num_triangles;
    sphereStream >> num_triangles;

    GLfloat x, y, z;
    int num_vertices;

    // record the maximum absolute coorinate
    for (int i = 0; i < num_triangles; ++i) {
        sphereStream >> num_vertices;
        for (int i = 0; i < num_vertices; ++i) {
            sphereStream >> x >> y >> z;
            sphere_points.emplace_back(x, y, z, 1);
            sphere_colors.emplace_back(1.0, 0.84, 0, 1);
            shadow_colors.emplace_back(0.25, 0.25, 0.25, 0.65);
        }
        vec4 vert1 = sphere_points[sphere_points.size() - 3];
        vec4 vert2 = sphere_points[sphere_points.size() - 2];
        vec4 vert3 = sphere_points[sphere_points.size() - 1];
        vec3 normal = normalize(cross(vert2 - vert1, vert3 - vert1));
        sphere_normals.insert(sphere_normals.end(), num_vertices, normal);
    }
    sphereStream.close();
}

// Set up lighting parameters that are uniform variables in shader.
// Note: "LightPosition" in shader must be in the Eye Frame.
//       So we use parameter "mv", the model-view matrix, to transform
//       light_position to the Eye Frame.
void updateLightingUniformVars(mat4 mv) {
    /*----- Shader Lighting Parameters -----*/
    glUniform4fv(glGetUniformLocation(program, "GlobalAmbient"), 1,
                 vec4(1.0, 1.0, 1.0, 1.0));
    glUniform4fv(glGetUniformLocation(program, "PositionalAmbient"), 1,
                 vec4(0.0, 0.0, 0.0, 1.0));
    glUniform4fv(glGetUniformLocation(program, "PositionalDiffuse"), 1,
                 vec4(1.0, 1.0, 1.0, 1.0));
    glUniform4fv(glGetUniformLocation(program, "PositionalSpecular"), 1,
                 vec4(1.0, 1.0, 1.0, 1.0));
    glUniform4fv(glGetUniformLocation(program, "DistantAmbient"), 1,
                 vec4(0.0, 0.0, 0.0, 1.0));
    glUniform4fv(glGetUniformLocation(program, "DistantDiffuse"), 1,
                 vec4(0.8, 0.8, 0.8, 1.0));
    glUniform4fv(glGetUniformLocation(program, "DistantSpecular"), 1,
                 vec4(0.2, 0.2, 0.2, 1.0));

    glUniform1f(glGetUniformLocation(program, "ConstAtt"), 2.0);
    glUniform1f(glGetUniformLocation(program, "LinearAtt"), 0.01);
    glUniform1f(glGetUniformLocation(program, "QuadAtt"), 0.001);
    glUniform1f(glGetUniformLocation(program, "Shininess"), 125.0);
    glUniform1f(glGetUniformLocation(program, "SpotlightExponent"), 15.0);

    // The Light Position in Eye Frame
    vec4 light_position_eyeFrame = mv * light_position;
    vec4 spotlight_destination_eyeFrame = mv * spotlight_destination;
    glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1,
                 light_position_eyeFrame);
    glUniform4fv(glGetUniformLocation(program, "SpotlightDestination"), 1,
                 spotlight_destination_eyeFrame);
    glUniform4fv(glGetUniformLocation(program, "DistantLightDirection"), 1,
                 vec4(0.1, 0.0, -1.0, 0.0));

    glUniform4fv(glGetUniformLocation(program, "MaterialAmbient"), 1,
                 material_ambient);
    glUniform4fv(glGetUniformLocation(program, "MaterialDiffuse"), 1,
                 material_diffuse);
    glUniform4fv(glGetUniformLocation(program, "MaterialSpecular"), 1,
                 material_specular);
    glUniform1i(glGetUniformLocation(program, "Spotlight"), spotlight);
}

// OpenGL initialization
void init() {
    read_file();

    // Create and initialize a vertex buffer object for sphere, to be used in
    // display()
    glGenBuffers(1, &sphere_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);

    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec4) * (sphere_points.size() + sphere_colors.size()) +
                     sizeof(vec3) * sphere_normals.size(),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * sphere_points.size(),
                    sphere_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * sphere_points.size(),
                    sizeof(vec4) * sphere_colors.size(), sphere_colors.data());
    glBufferSubData(
        GL_ARRAY_BUFFER,
        sizeof(vec4) * (sphere_points.size() + sphere_colors.size()),
        sizeof(vec3) * sphere_normals.size(), sphere_normals.data());

    floor();

    // Create and initialize a vertex buffer object for floor, to be used in
    // display()
    glGenBuffers(1, &floor_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec4) * (floor_points.size() + floor_colors.size()) +
                     sizeof(vec3) * floor_normals.size(),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * floor_points.size(),
                    floor_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * floor_points.size(),
                    sizeof(vec4) * floor_colors.size(), floor_colors.data());
    glBufferSubData(GL_ARRAY_BUFFER,
                    sizeof(vec4) * (floor_points.size() + floor_colors.size()),
                    sizeof(vec3) * floor_normals.size(), floor_normals.data());

    axis();

    // Create and initialize a vertex buffer object for axis, to be used in
    // display()
    glGenBuffers(1, &axis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec4) * (axis_points.size() + axis_colors.size()), NULL,
                 GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * axis_points.size(),
                    axis_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * axis_points.size(),
                    sizeof(vec4) * axis_colors.size(), axis_colors.data());

    // Create and initialize a vertex buffer object for shadow, to be used in
    // display()
    glGenBuffers(1, &shadow_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, shadow_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec4) * (sphere_points.size() + shadow_colors.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * sphere_points.size(),
                    sphere_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * sphere_points.size(),
                    sizeof(vec4) * shadow_colors.size(), shadow_colors.data());

    // Load shaders and create a shader program (to be used in display())
    program = InitShader("vshader53.vert", "fshader53.frag");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.529, 0.807, 0.92, 0.0);
}

// draw the object that is associated with the vertex buffer object "buffer"
// and has "num_vertices" vertices.
// Mode: GL_LINE or GL_TRIANGLES
void drawObj(GLuint buffer, int num_vertices, GLenum mode,
             GLboolean objLighting = GL_FALSE,
             GLboolean objSmoothShading = GL_FALSE) {
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    glUniform1i(glGetUniformLocation(program, "Lighting"), objLighting);
    glUniform1i(glGetUniformLocation(program, "SmoothShading"),
                objSmoothShading);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0));

    if (objLighting == GL_FALSE) {
        GLuint vColor = glGetAttribLocation(program, "vColor");
        glEnableVertexAttribArray(vColor);
        glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
                              BUFFER_OFFSET(sizeof(vec4) * num_vertices));

        glDrawArrays(mode, 0, num_vertices);

        /*--- Disable each vertex attribute array being enabled ---*/
        glDisableVertexAttribArray(vPosition);
        glDisableVertexAttribArray(vColor);
    } else {
        GLuint vNormal = glGetAttribLocation(program, "vNormal");
        glEnableVertexAttribArray(vNormal);
        glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                              BUFFER_OFFSET(sizeof(vec4) * num_vertices * 2));

        glDrawArrays(mode, 0, num_vertices);

        /*--- Disable each vertex attribute array being enabled ---*/
        glDisableVertexAttribArray(vPosition);
        glDisableVertexAttribArray(vNormal);
    }
}

void display(void) {
    GLuint ModelView;   // model-view matrix uniform shader variable location
    GLuint Projection;  // projection matrix uniform shader variable location

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);  // Use the shader program

    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");

    /*---  Set up and pass on Projection matrix to the shader ---*/
    mat4 p = Perspective(fovy, aspect, zNear, zFar);
    // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(Projection, 1, GL_TRUE, p);

    /*---  Set up and pass on Model-View matrix to the shader ---*/
    // eye is a global variable of vec4 set to init_eye and updated by
    // keyboard()
    vec4 at(0.0, 0.0, 0.0, 1.0);
    vec4 up(0.0, 1.0, 0.0, 0.0);

    mat4 mv = LookAt(eye, at, up);

    drawObj(axis_buffer, axis_points.size(), GL_LINES);  // draw the axis

    /*----- Set Up the Model-View matrix for the sphere -----*/
    int next_pt_index = (curr_pt_index + 1) % num_pts;
    vec4 direction = normalize(rolling_points[next_pt_index] -
                               rolling_points[curr_pt_index]);
    vec4 rotation_axis = vec4(cross(vec4(0, 1, 0, 0), direction), 0);
    GLfloat distance = 2 * M_PI * angle / 360;
    vec4 from_curr_pt = direction * distance;
    // The set-up below gives a new scene (scene 2), using
    // Correct LookAt().
    mat4 mv_sphere =
        mv * Translate(rolling_points[curr_pt_index] + from_curr_pt) *
        Rotate(angle, rotation_axis[0], rotation_axis[1], rotation_axis[2]) *
        accumulated_rotation;

    // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv_sphere);

    // Set up the Normal Matrix from the model-view matrix
    mat3 normal_matrix = NormalMatrix(mv_sphere, 0);
    glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1,
                       GL_TRUE, normal_matrix);

    /*--- Set up lighting parameters that are uniform variables in shader ---*/
    material_ambient = vec4(0.2, 0.2, 0.2, 1.0);
    material_diffuse = vec4(1.0, 0.84, 0.0, 1.0);
    material_specular = vec4(1.0, 0.84, 0.0, 1.0);
    updateLightingUniformVars(mv);

    if (solidSphere) {  // Filled sphere
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawObj(sphere_buffer, sphere_points.size(), GL_TRIANGLES, lighting,
                smoothShading);
    } else {  // Wireframe sphere
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawObj(sphere_buffer, sphere_points.size(), GL_TRIANGLES);
    }

    /*----- Set up the Mode-View matrix for the floor -----*/
    // Draw floor for the first time, write to frame buffer only
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv);

    // Set up the Normal Matrix from the model-view matrix
    normal_matrix = NormalMatrix(mv, 0);
    glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1,
                       GL_TRUE, normal_matrix);

    /*--- Set up lighting parameters that are uniform variables in shader ---*/
    material_ambient = vec4(0.2, 0.2, 0.2, 1.0);
    material_diffuse = vec4(0.0, 1.0, 0.0, 1.0);
    material_specular = vec4(0.0, 0.0, 0.0, 1.0);
    updateLightingUniformVars(mv);

    glDepthMask(GL_FALSE);
    if (solidFloor) {  // Filled floor
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawObj(floor_buffer, floor_points.size(), GL_TRIANGLES, lighting);
    } else {  // Wireframe floor
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawObj(floor_buffer, floor_points.size(), GL_TRIANGLES);
    }
    glDepthMask(GL_TRUE);

    if (shadow && eye.y > 0) {
        /*----- Set Up the Model-View matrix for the shadow -----*/
        // GL_TRUE: matrix is row-major
        mat4 shadow_projection(light_position.y, 0, 0, 0, -light_position.x, 0,
                               -light_position.z, -1, 0, 0, light_position.y, 0,
                               0, 0, 0, light_position.y);

        mat4 mv_shadow =
            mv * shadow_projection *
            Translate(rolling_points[curr_pt_index] + from_curr_pt) *
            Rotate(angle, rotation_axis[0], rotation_axis[1],
                   rotation_axis[2]) *
            accumulated_rotation;

        glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv_shadow);
        if (solidSphere)  // Filled sphere
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        else  // Wireframe sphere
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawObj(shadow_buffer, shadow_colors.size(),
                GL_TRIANGLES);  // draw the shadow
    }

    // draw floor for the second time, write floor to z-buffer only
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv);
    if (solidFloor)  // Filled floor
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else  // Wireframe floor
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    drawObj(floor_buffer, floor_points.size(), GL_TRIANGLES);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glutSwapBuffers();
}

void idle(void) {
    angle += 2.0;
    GLfloat distance = 2 * M_PI * angle / 360;
    int next_pt_index = (curr_pt_index + 1) % num_pts;
    if (distance >=
        length(rolling_points[next_pt_index] - rolling_points[curr_pt_index])) {
        vec4 direction = normalize(rolling_points[next_pt_index] -
                                   rolling_points[curr_pt_index]);
        vec4 rotation_axis = vec4(cross(vec4(0, 1, 0, 0), direction), 0);
        accumulated_rotation = Rotate(angle, rotation_axis[0], rotation_axis[1],
                                      rotation_axis[2]) *
                               accumulated_rotation;
        curr_pt_index = next_pt_index;
        angle = 0;
    }
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 033:  // Escape Key
        case 'q':
        case 'Q':
            exit(EXIT_SUCCESS);
            break;

        case 'X':
            eye[0] += 1.0;
            break;
        case 'x':
            eye[0] -= 1.0;
            break;
        case 'Y':
            eye[1] += 1.0;
            break;
        case 'y':
            eye[1] -= 1.0;
            break;
        case 'Z':
            eye[2] += 1.0;
            break;
        case 'z':
            eye[2] -= 1.0;
            break;

        case 'b':
        case 'B':  // Toggle between animation and non-animation
            if (!b_key_pressed) b_key_pressed = true;
            animation = !animation;
            if (animation)
                glutIdleFunc(idle);
            else
                glutIdleFunc(NULL);
            break;

        case 'c':
        case 'C':  // Toggle between filled and wireframe sphere
            solidSphere = !solidSphere;
            break;

        case 'f':
        case 'F':  // Toggle between filled and wireframe floor
            solidFloor = !solidFloor;
            break;

        case ' ':  // reset to initial viewer/eye position
            eye = init_eye;
            break;
    }
    glutPostRedisplay();
}

void mainMenu(int id) {
    switch (id) {
        case 0:
            eye = init_eye;
            break;
        case 1:
            exit(EXIT_SUCCESS);
            break;
        case 2:
            shadow = true;
            break;
        case 3:
            shadow = false;
            break;
        case 4:
            lighting = true;
            break;
        case 5:
            lighting = false;
            break;
        case 6:
            spotlight = true;
            break;
        case 7:
            spotlight = false;
            break;
        case 8:
            smoothShading = false;
            break;
        case 9:
            smoothShading = true;
            break;
        case 10:
            solidSphere = false;
            break;
        case 11:
            solidSphere = true;
            break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        int shadow_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("On", 2);
        glutAddMenuEntry("Off", 3);
        int lighting_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("On", 4);
        glutAddMenuEntry("Off", 5);
        int lightsource_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Spotlight", 6);
        glutAddMenuEntry("Point Source", 7);
        int shading_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Flat Shading", 8);
        glutAddMenuEntry("Smooth Shading", 9);
        int wireframe_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Wireframe", 10);
        glutAddMenuEntry("Solid", 11);
        glutCreateMenu(mainMenu);
        glutAddMenuEntry("Default View Point", 0);
        glutAddSubMenu("Shadow", shadow_menu);
        glutAddSubMenu("Lighting", lighting_menu);
        glutAddSubMenu("Light Source", lightsource_menu);
        glutAddSubMenu("Shading", shading_menu);
        glutAddSubMenu("Sphere Type", wireframe_menu);
        glutAddMenuEntry("Quit", 1);
        glutAttachMenu(GLUT_LEFT_BUTTON);
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
        if (b_key_pressed) {
            animation = !animation;
            if (animation)
                glutIdleFunc(idle);
            else
                glutIdleFunc(NULL);
        }
    }
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    aspect = (GLfloat)width / (GLfloat)height;
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
#ifdef __APPLE__  // Enable core profile of OpenGL 3.2 on macOS.
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH |
                        GLUT_3_2_CORE_PROFILE);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutInitWindowSize(512, 512);
    glutCreateWindow("Rolling Sphere");

#ifdef __APPLE__  // on macOS
    // Core profile requires to create a Vertex Array Object (VAO).
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#else  // on Linux or Windows, we still need glew
    /* Call glewInit() and error checking */
    int err = glewInit();
    if (GLEW_OK != err) {
        printf("Error: glewInit failed: %s\n", (char*)glewGetErrorString(err));
        exit(1);
    }
#endif

    // Get info of GPU and supported OpenGL version
    cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL version supported " << glGetString(GL_VERSION) << endl;

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);

    init();
    glutMainLoop();
    return 0;
}
