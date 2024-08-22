
// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <algorithm>
#include <list>
#include <set>
#include <iostream>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

//audio
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "collisions.hpp"
// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loops de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// New classes
class PhysicsObject;
class Rect;


// new global variables
int global_Object_Index = 0;
int global_Text_Line;

float g_encacapada_anim_speed = 1.0f;
float collision_t;

bool g_TapFlag = false;
bool g_FreeCamera = true;

glm::vec4 g_LookAt_Coords;
glm::vec4 g_Camera_LookAt;

glm::vec4 g_POV_Coords = glm::vec4(3.0f, 1.7f, 3.0f, 1.0f);
glm::vec4 g_POV_LookAt;
float g_free_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_free_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y

glm::vec4 g_FinalCameraCoords;
glm::vec4 g_FinalCameraLookAtCoords;

float cameraBezierT = 0;
float g_ball_radius = 0.03f;
float hole_width = 0.09f;

// vars
float g_recoilAnim = 0;
float g_zoomAnim = 0;
float g_ak_downtime = 0;
float g_ak_downtime_const = 0.1;

glm::vec4 up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

std::list<PhysicsObject> PhysicsObjects;

 float xPlusBound = 2.25f * 0.5f;
float xMinusBound = 1.72f * -0.5f;
float zPlusBound = 1.00f * 0.5f;
float zMinusBound = 1.05f * -0.5f;
float yPlusBound = 20;
float yMinusBound = 0.98;

glm::vec4 player_dim =      glm::vec4(0.1f, 1.75f, 0.1f, 0.0f);
glm::vec4 table_coords =    glm::vec4((xPlusBound + xMinusBound)/2, 0.0f, (zPlusBound + zMinusBound)/2, 1.0f);
glm::vec4 table_dim =       glm::vec4(xPlusBound, 1.0f, zPlusBound + 0.0f, 0.0f);

bool w_held, a_held, s_held, d_held, shift_held, ctrl_held = false;

float walk_speed = 2.0f;
bool opening_shot = true;
float opening_multiplier = 5.0f;

int gunType = 0;

//New functions

//DRAWING SPHERES
void DrawSphere(glm::vec4 position, float radius, int index);
void DrawSphereCoords(int x, int y, int z, float radius);
// Time
float ellapsed_time();

// Collisions
float t_colision_sphere_plane(PhysicsObject sphere, char axis, int direction, float offset);
bool collideSpheres(PhysicsObject * o1, PhysicsObject * o2);

//utility
float distance(PhysicsObject o1, PhysicsObject o2);
float dist(glm::vec4 v1, glm::vec4 v2);
glm::vec4 getVectorBetween(PhysicsObject o1, PhysicsObject o2);
glm::vec4 planarize(glm::vec4 v);
glm::vec4 LERP(glm::vec4 p1, glm::vec4 p2, float t);
float LERP(float f1, float f2, float t);
glm::vec4 Bezier(glm::vec4 p1, glm::vec4 p2, glm::vec4 p3, glm::vec4 p4, float t);
float Bezier(float f1, float f2, float f3, float f4, float t);


void resetBalls();


// CLASSES
// Scene Objects
// +obj ff3
class PhysicsObject {       
  public:             
    int index;        
    float radius;  
    float mass;
    float encacapada_animation_t;
    std::string type;
    glm::vec4 position;
    glm::vec4 movement_vector;
    glm::vec4 hole_coords;
    glm::vec4 pre_encacapada_position;
    glm::mat4 rotation_matrix = Matrix_Identity();
    

    PhysicsObject(std::string t, float r, float m, float x, float y, float z) { // Constructor with parameters
      index = global_Object_Index;
      global_Object_Index++;
      type = t;
      radius = r;
      position = glm::vec4(x, y, z, 1.0f);
      mass = m;
      encacapada_animation_t = -1;
    }

    void setMovementVector(float x, float y, float z){
        movement_vector = glm::vec4(x, y, z, 0.0f);
    }

    void draw(){
            glm::mat4 model = Matrix_Translate(position.x, position.y, position.z) 
                * Matrix_Scale(radius, radius, radius)
                * rotation_matrix;
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, (index % 15) + 10);
            DrawVirtualObject("the_sphere");
            
    }

    void advance_time(float dt){
        
        // Update position
        position = position + movement_vector * dt;

        // Update angle

        

        if(length(planarize(movement_vector)) > 0.1 ){
            // math
            
            glm::vec4 weird_vector = normalize(glm::vec4(1.0f, 2.0f, 1.0f, 0.0f));

            rotateByAxis(weird_vector, length(movement_vector) * dt * 10.0f);        
        }
    }

    

    void reflectAxis(char axis){
        float directional_loss = 0.5;
        if(axis == 'x'){
            movement_vector.x = -movement_vector.x * directional_loss;
        } else if (axis == 'y'){
            movement_vector.y = -movement_vector.y * directional_loss;
        }else if (axis == 'z'){
            movement_vector.z = -movement_vector.z * directional_loss;
        } else{
            printf("Sintax error -> reflectAxis: axis doesnt exist");
        }
    }

    void reflectAxis(char axis, float directional_loss){
        if(axis == 'x'){
            movement_vector.x = -movement_vector.x * directional_loss;
        } else if (axis == 'y'){
            movement_vector.y = -movement_vector.y * directional_loss;
        }else if (axis == 'z'){
            movement_vector.z = -movement_vector.z * directional_loss;
        } else{
            printf("Sintax error -> reflectAxis: axis doesnt exist");
        }
    }

    void reflectNormal(glm::vec4 vector){
        glm::vec4 normal = normalize(vector);
        glm::vec4 new_movement_vector = movement_vector - 2 * dotproduct(movement_vector, normal) * normal;
        movement_vector = new_movement_vector;
    }

    void collideWithBounds(float xPlusBound, float xMinusBound, float zPlusBound, float zMinusBound, float yPlusBound, float yMinusBound){
        if(position.x + radius > xPlusBound){
            position.x = position.x + (xPlusBound - position.x - radius);
            reflectAxis('x');
        }
        if(position.x - radius < xMinusBound){
            position.x = position.x + (xMinusBound - position.x + radius);
            reflectAxis('x');
        }
        if(position.z + radius > zPlusBound){
            position.z = position.z + (zPlusBound - position.z - radius);
            reflectAxis('z');
        }
        if(position.z - radius < zMinusBound){
            position.z = position.z + (zMinusBound - position.z + radius);
            reflectAxis('z');
        }

        if(position.y + radius > yPlusBound){
            position.y = position.y + (yPlusBound - position.y - radius);
            reflectAxis('y');
        }
       
    }

    void collideWithFloor(float tableHeight){

        // Floor collision
        //if is in a hole, ignore regular floor collision
        
        if((position.y - radius < tableHeight)){
            position.y = position.y + (tableHeight - position.y + radius);
            reflectAxis('y', 0.2);
        }
    }

    
    bool collideWithHole(glm::vec4 hole, float tableHeight){

        // Collide with the bottom of the hole
        float holeBottomY = tableHeight - 1.5f;
        if(position.y - radius < holeBottomY){
            position.y = position.y + (holeBottomY - position.y + radius);
            reflectAxis('y', 0.2);
        }

        if(point_cillinder_collide(position, hole, hole_width)){

            
            if( dist(planarize(position) , planarize(hole))< (hole_width - radius)){
            } else {
                glm::vec4 dir = normalize( planarize(position) - planarize(hole));
                glm::vec4 contact = hole + dir * hole_width;
                if(position.y <= tableHeight){
                    contact.y = position.y;
                } 
                float offset = dist(position, contact) - radius;
                if(offset <= 0){
                    //Collision with hole edge
                    position = position + dir * offset;
                    reflectNormal(contact - position);
                }
                

            }
            return true;
        } else {
            // Not touching hole
            return false;
        }

        

    }

    void applyStaticFriction(float coef){
        glm::vec4 friction_vector = -normalize(movement_vector) * coef;

        if(length(friction_vector) >= length(movement_vector)){
            movement_vector = movement_vector + friction_vector;
        } else {
            movement_vector = movement_vector + friction_vector;
        }
    }

    void encacapar(glm::vec4 hole_coords){
        encacapada_animation_t = 0;
        movement_vector = glm::vec4(0,0,0,0);
        pre_encacapada_position = position;
        hole_coords = hole_coords;
    }

    void play_encacapa_animation(){
        position = Bezier(pre_encacapada_position, pre_encacapada_position + movement_vector, hole_coords + glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), hole_coords, encacapada_animation_t);
    }


    void rotateByAxis(glm::vec4 axis, float angle){

        rotation_matrix = rotation_matrix * Matrix_Axis_Rotation2(axis, angle);

    }
};


class Rect {       
  public:             
    float x;
    float z;   
    float x_width;
    float z_width;

    Rect(float x, float z, float x_width, float z_width) { // Constructor with parameters
        x = x;
        z = z;
        x_width = x_width;
        z_width = z_width;

    }

    glm::vec4 toFirstPerson(){
        return glm::vec4(x, 1.0f, z, 1.0f);
    }
};


Rect playerRect = Rect(-3.0f, 0.0f, 0.3f, 0.3f);
Rect tableRect = Rect(0.0f, 0.0f, 2.0f, 2.0f);


int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "Sinuca Balistica", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);


    // Hides cursor -> doesnt work for some reason
    glfwSetInputMode(window, GLFW_CURSOR , GLFW_CURSOR_DISABLED);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/textures/P88_gloss.jpg"); // TextureTableTop:
    LoadTextureImage("../../data/textures/pool table low_POOL TABLE_BaseColor.png"); // TexturePoolTable:
    LoadTextureImage("../../data/textures/P88_gloss.jpg"); // TextureObjUnkown:

    LoadTextureImage("../../data/textures/balls/BallCue.jpg"); // TextureCueBall:
    LoadTextureImage("../../data/textures/balls/Ball1.jpg"); // TextureBall1:
    LoadTextureImage("../../data/textures/balls/Ball2.jpg"); // TextureBall2:
    LoadTextureImage("../../data/textures/balls/Ball3.jpg"); // TextureBall3:
    LoadTextureImage("../../data/textures/balls/Ball4.jpg"); // TextureBall4:
    LoadTextureImage("../../data/textures/balls/Ball5.jpg"); // TextureBall5:
    LoadTextureImage("../../data/textures/balls/Ball6.jpg"); // TextureBall6:
    LoadTextureImage("../../data/textures/balls/Ball7.jpg"); // TextureBall7:
    LoadTextureImage("../../data/textures/balls/Ball8.jpg"); // TextureBall8:
    LoadTextureImage("../../data/textures/balls/Ball9.jpg"); // TextureBall9:
    LoadTextureImage("../../data/textures/balls/Ball10.jpg"); // TextureBall10:
    LoadTextureImage("../../data/textures/balls/Ball11.jpg"); // TextureBall11:
    LoadTextureImage("../../data/textures/balls/Ball12.jpg"); // TextureBall12:
    LoadTextureImage("../../data/textures/balls/Ball13.jpg"); // TextureBall13:
    LoadTextureImage("../../data/textures/balls/Ball14.jpg"); // TextureBall14:
    LoadTextureImage("../../data/textures/balls/Ball15.jpg"); // TextureBall15:

    LoadTextureImage("../../data/brick_room/material_diffuse.jpeg"); // BrickRoom:
    LoadTextureImage("../../data/ak-47/mat0_c.jpeg"); // ak47:


    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel gunmodel("../../data/Gun.obj");
    ComputeNormals(&gunmodel);
    BuildTrianglesAndAddToVirtualScene(&gunmodel);

    ObjModel tabletopmodel("../../data/POOL TABLE.obj");
    ComputeNormals(&tabletopmodel);
    BuildTrianglesAndAddToVirtualScene(&tabletopmodel);

    ObjModel brickroommodel("../../data/brick_room/basement.obj");
    ComputeNormals(&brickroommodel);
    BuildTrianglesAndAddToVirtualScene(&brickroommodel);    

    ObjModel ak47model("../../data/ak-47/ak-47.obj");
    ComputeNormals(&ak47model);
    BuildTrianglesAndAddToVirtualScene(&ak47model);  

    
    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);


//==========================================================================||
//||                                                                        ||
//||                  Sounds                                                ||
//||                                                                 +snd   ||
//==========================================================================||

    ma_result result;
    ma_engine engine;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        std::cout << "Audio engine not working" << std::endl;
    } else {
        std::cout << "Audio engine is working" << std::endl;
    }

    ma_sound gunshot_sound;
   

    result = ma_sound_init_from_file(&engine, "../../sounds/gunshot.mp3", 0, NULL, NULL, &gunshot_sound);
    if (result != MA_SUCCESS) {
        std::cout << "Audio  not working" << std::endl;
        std::cout << result << std::endl;
        printf("error: %d, \n", result);
    } else {
        std::cout << "Audio is working" << std::endl;

        }
     ma_sound_set_volume(&gunshot_sound, 0.02f);

    ma_sound clack_sound;

    result = ma_sound_init_from_file(&engine, "../../sounds/clack.mp3", 0, NULL, NULL, &clack_sound);
    ma_sound_set_volume(&clack_sound, 0.2f);

//==========================================================================||
//||                                                                        ||
//||                  Scene creation i guess                         ff2    ||
//||                                                                 +set   ||
//==========================================================================||
    float run_time = (float)glfwGetTime();

    std::list<glm::vec4> Holes;
    Holes.push_back(glm::vec4(xPlusBound, yMinusBound, zPlusBound, 1.0f));
    Holes.push_back(glm::vec4(xPlusBound, yMinusBound, zMinusBound, 1.0f));
    Holes.push_back(glm::vec4(xMinusBound, yMinusBound, zPlusBound, 1.0f));
    Holes.push_back(glm::vec4(xMinusBound, yMinusBound, zMinusBound, 1.0f));
    Holes.push_back(glm::vec4((xPlusBound + xMinusBound) / 2, yMinusBound, zPlusBound, 1.0f));
    Holes.push_back(glm::vec4((xPlusBound + xMinusBound) / 2, yMinusBound, zMinusBound, 1.0f));
  
    float x = -0.3f;
    float y = yMinusBound;
    float z = (zPlusBound + zMinusBound) /2;
    float diameter = g_ball_radius * 2 + 0.01f;


    PhysicsObject b1 = PhysicsObject("ball", g_ball_radius, 30.0f, 0.7f, 0.0, z);
    b1.setMovementVector(0.0f, 0.0f, 0.1f);
    PhysicsObjects.push_back(b1);

    for(int i = 0; i < 5; i++){
        for(int j = 0; j < i; j++){

            PhysicsObject b = PhysicsObject("ball", g_ball_radius, 30.0f, x, y, z);
            b.setMovementVector(0.0f, 0.0f, 0.1f);
            PhysicsObjects.push_back(b);

            z = z + diameter ;
        }
         z = z - (diameter * (i + 0.5f));
        x = x - diameter * sqrt(3) / 2;
    }

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {

        float delta_t = (float)glfwGetTime() - run_time;
        run_time = (float)glfwGetTime();

        

        global_Text_Line = 2;
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

       

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

 //==========================================================================||
//||                                                                        ||
//||                  Camera code                                  ff4      ||
//||                                                               +cam     ||
//==========================================================================||


        if(!g_FreeCamera){
            if(cameraBezierT < 1){
                cameraBezierT = cameraBezierT + delta_t;
            } else {
                cameraBezierT = 1;
            }
        } else {
            if(cameraBezierT > 0){
                cameraBezierT = cameraBezierT - delta_t;
            } else {
                cameraBezierT = 0;
            }
        }

        if(g_RightMouseButtonPressed){
            if(g_zoomAnim < 1){
                g_zoomAnim = g_zoomAnim + delta_t * 3;
            } else {
                g_zoomAnim = 1;
            }
        } else {
            if(g_zoomAnim > 0){
                g_zoomAnim = g_zoomAnim - delta_t * 3;
            } else {
                g_zoomAnim = 0;
            }
        }     
        
        g_Camera_LookAt = PhysicsObjects.front().position; 

        float r = g_CameraDistance;
        float y = g_Camera_LookAt.y + r*sin(g_CameraPhi);
        float z = g_Camera_LookAt.z + r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = g_Camera_LookAt.x + r*cos(g_CameraPhi)*sin(g_CameraTheta);

        g_LookAt_Coords = glm::vec4(x,y,z,1.0f);

        g_POV_LookAt = g_POV_Coords + glm::vec4(cos(-g_free_CameraTheta)*cos(g_free_CameraPhi), -sin(g_free_CameraPhi), sin(-g_free_CameraTheta)*cos(g_free_CameraPhi),0.0f);
        

        g_FinalCameraCoords = Bezier(g_POV_Coords, g_POV_Coords + glm::vec4(0.0f,1.0f,0.0f,0.0f), g_LookAt_Coords +  glm::vec4(0.0f,1.0f,0.0f,0.0f), g_LookAt_Coords ,cameraBezierT);
        g_FinalCameraLookAtCoords = Bezier(g_POV_LookAt, g_POV_LookAt, g_Camera_LookAt, g_Camera_LookAt, cameraBezierT);

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::vec4 camera_position_c;
        glm::vec4 camera_lookat_l;


        camera_position_c  = g_FinalCameraCoords;
        camera_lookat_l    = g_FinalCameraLookAtCoords;

        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

        glm::vec4 camera_side_vector = crossproduct(camera_view_vector, camera_up_vector);
        glm::vec4 camera_side_vector_normalized =  normalize(camera_side_vector);
        glm::vec4 camera_horizontal_forward_vector = planarize(camera_view_vector);
        glm::vec4 camera_horizontal_normalized = normalize(camera_horizontal_forward_vector);


        float zoom_slowdown = Bezier(1, 1, 0.3, 0.3, g_zoomAnim);

        if(w_held){
            g_POV_Coords = g_POV_Coords + camera_horizontal_normalized * walk_speed * zoom_slowdown * delta_t;
            }
        if(s_held){
            g_POV_Coords = g_POV_Coords - camera_horizontal_normalized * walk_speed * zoom_slowdown * delta_t;
            }
        if(a_held){
            g_POV_Coords = g_POV_Coords - camera_side_vector_normalized * walk_speed * zoom_slowdown * delta_t;
            }
        if(d_held){
            g_POV_Coords = g_POV_Coords + camera_side_vector_normalized * walk_speed * zoom_slowdown * delta_t;
            }
        if(shift_held){
            g_POV_Coords = g_POV_Coords + camera_up_vector * 1.5f * delta_t;
            }
        if(ctrl_held){
            g_POV_Coords = g_POV_Coords - camera_up_vector * 1.5f * delta_t;
            }


        bool isCollidingWithTable = collision_box_box(g_POV_Coords, table_coords, player_dim, table_dim);

        
        // top 10 worst code ever
        if(isCollidingWithTable){
            if(w_held){
                g_POV_Coords = g_POV_Coords - camera_horizontal_normalized * walk_speed * zoom_slowdown * delta_t;
                }
            if(s_held){
                g_POV_Coords = g_POV_Coords + camera_horizontal_normalized * walk_speed * zoom_slowdown * delta_t;
                }
            if(a_held){
                g_POV_Coords = g_POV_Coords + camera_side_vector_normalized * walk_speed * zoom_slowdown * delta_t;
                }
            if(d_held){
                g_POV_Coords = g_POV_Coords - camera_side_vector_normalized * walk_speed * zoom_slowdown * delta_t;
                }
        }



        if(g_POV_Coords.x > 4.8f){
            g_POV_Coords.x = 4.8f;
        }

        if(g_POV_Coords.x < -4.8f){
            g_POV_Coords.x = -4.8f;
        }

        if(g_POV_Coords.z > 4.2f){
            g_POV_Coords.z = 4.2f;
        }

        if(g_POV_Coords.z < -5.0f){
            g_POV_Coords.z = -5.0f;
        }
        
        
        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.01f;  // Posição do "near plane"
        float farplane  = -1000.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = (3.141592 / 3.0f) * Bezier(1, 1 , 0.3,  0.3, g_zoomAnim); 

            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        
        #define SPHERE 0
        #define GUN 3
        #define TABLE_TOP 4
        #define UNKNOWN -2
        #define BRICK_ROOM 21
        #define AK47 26    

        // Desenhamos A mesa
        model = Matrix_Translate(0.0f,0.0f,0.0f) * Matrix_Scale(1.0f, 1.0f, 1.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, TABLE_TOP);
        DrawVirtualObject("Base_low_Mesh.024");
        DrawVirtualObject("Box14_low_Mesh.022");
        DrawVirtualObject("feet_low_Mesh.020");
        DrawVirtualObject("legs_low_Mesh.019");
        DrawVirtualObject("rubber_low_Mesh.018");
        DrawVirtualObject("tabletop_low_Mesh.013");


        model = Matrix_Translate(0.0f,0.0f,0.0f) * Matrix_Scale(1.0f, 1.0f, 1.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, BRICK_ROOM);
        DrawVirtualObject("group_3_ID27");
        DrawVirtualObject("group_4_ID64");
        DrawVirtualObject("group_5_ID71");
        DrawVirtualObject("group_6_ID78");
        DrawVirtualObject("group_7_ID91");
        DrawVirtualObject("group_8_ID104");

        // Desenhamos o plano da arma

        if(gunType == 0){
            model = Matrix_Translate(g_POV_Coords.x, g_POV_Coords.y, g_POV_Coords.z)
            * Matrix_Rotate_X(0.0f)
            * Matrix_Rotate_Y(g_free_CameraTheta + (3.14))
            * Matrix_Rotate_Z(g_free_CameraPhi + -LERP(0, (3.14 / LERP(5, 20, g_zoomAnim)) ,g_recoilAnim))
            * Matrix_Translate(
            LERP(-0.15f, -0.3f, g_zoomAnim),
            LERP(-0.2f, -0.18f, g_zoomAnim),
            LERP(-0.1f, 0.0f, g_zoomAnim))
            
            * Matrix_Scale(0.01f, 0.01f, 0.01f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, GUN);
            DrawVirtualObject("P88");
        } else if(gunType == 1){
            // ak 47
            model = Matrix_Translate(g_POV_Coords.x, g_POV_Coords.y, g_POV_Coords.z)
            * Matrix_Rotate_X(0.0f)
            * Matrix_Rotate_Y(g_free_CameraTheta + (3.14))
            * Matrix_Rotate_Z(g_free_CameraPhi + -LERP(0, (3.14 / LERP(30, 50, g_zoomAnim)) ,g_recoilAnim))
            * Matrix_Translate(
            LERP(-0.25f, -0.35f, g_zoomAnim),
            LERP(-0.25f, -0.20f, g_zoomAnim),
            LERP(-0.1f, 0.0f, g_zoomAnim))
            * Matrix_Rotate_Y(3.141f)
            * Matrix_Scale(0.05f, 0.05f, 0.05f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, AK47);
            DrawVirtualObject("magazine_low_0");
            DrawVirtualObject("stock_low_0");
            DrawVirtualObject("rear_sight_low_0");
            DrawVirtualObject("pistolstock_low_0");
            DrawVirtualObject("upperreceiver_low_0");
            DrawVirtualObject("barrel_element_low_0");
            DrawVirtualObject("front_sight_big_cylinder_low_0");
            DrawVirtualObject("rear_sight_screw_low_0");
            DrawVirtualObject("safetyswitchscrew_low_0");
            DrawVirtualObject("safetyswitch_low_0");
            DrawVirtualObject("rear_sight_element_b_low_0");
            DrawVirtualObject("rear_sight_element_a_low_0");
            DrawVirtualObject("swivel_low_0");
            DrawVirtualObject("bolt_carrier_low_0");
            DrawVirtualObject("receiver_low_0");
            DrawVirtualObject("rear_sight_leaf_low_0");
            DrawVirtualObject("trigger_low_0");
            DrawVirtualObject("spring_low_0");
            DrawVirtualObject("rear_sight_switch_knob_low_0");
            DrawVirtualObject("rear_sight_switch_low_0");
            DrawVirtualObject("sylinder_low_0");
            DrawVirtualObject("barrel_cylinder_low_0");
            DrawVirtualObject("front_sight_needle_low_0");
            DrawVirtualObject("stock_element_low_0");
            DrawVirtualObject("front_sight_cylinder_low_0");
            DrawVirtualObject("rear_sight_element_screw_low_0");
            DrawVirtualObject("receiver_screws_low_0");
            DrawVirtualObject("rear_sight_cylinder_low_0");
            DrawVirtualObject("triggerguard_screws_low_0");
            DrawVirtualObject("triggerguard_low_0");
            DrawVirtualObject("magazine_catch_low_0");
            DrawVirtualObject("muzzlebreak_low_0");
            DrawVirtualObject("stock_screws_low_0");
            DrawVirtualObject("barrel_low_0");
            DrawVirtualObject("triggerguard_upper_low_0");
            DrawVirtualObject("gas_block_low_0");
            DrawVirtualObject("gas_cylinder_low_0");
            DrawVirtualObject("handguard_low_0");
            DrawVirtualObject("element_rear_sight_low_0");
            DrawVirtualObject("handguard_upper_low_0");
            DrawVirtualObject("handguardmetal_low_0");
            DrawVirtualObject("front_sight_low_0");
            DrawVirtualObject("polySurface228_0");
            DrawVirtualObject("polySurface229_0");
        }

//==========================================================================||
//||                                                                        ||
//||                  Intersting physics code                      ff1      ||
//||                                                               +phy     ||
//==========================================================================||



        glm::vec4 rayCastPoint;
        float rayCastDist;

        // Displaying bound of table
        DrawSphereCoords(xPlusBound,yMinusBound,zPlusBound,0.02f);
        DrawSphereCoords(xPlusBound,yMinusBound,zMinusBound,0.02f);
        DrawSphereCoords(xMinusBound,yMinusBound,zPlusBound,0.02f);
        DrawSphereCoords(xMinusBound,yMinusBound,zMinusBound,0.02f);

        for(PhysicsObject &object : PhysicsObjects){
            object.draw();

            bool inHole = false;
            if(object.index != 0) for(glm::vec4 hole : Holes){
                // returs tru if it is in currently tested hole
                // of if one of the previous tests was true]
                
                inHole = object.collideWithHole(hole, yMinusBound) || inHole;
            }
            if(!inHole){
                object.collideWithBounds(xPlusBound, xMinusBound, zPlusBound, zMinusBound, yPlusBound, yMinusBound);
                object.collideWithFloor(yMinusBound);
            }
            object.advance_time(delta_t);
            object.applyStaticFriction(0.6 * delta_t);
            
            object.movement_vector.y = object.movement_vector.y - 10.0f * delta_t;
            
        }

        //for each pair of objects
        for(PhysicsObject &o1 : PhysicsObjects){
            for(PhysicsObject &o2 : PhysicsObjects){
                if(&o1 != &o2 && &o1 < &o2){
                    if(collideSpheres(&o1, &o2)){
                        ma_sound_stop(&clack_sound);
                        ma_sound_seek_to_pcm_frame(&clack_sound, 0);
                        ma_sound_start(&clack_sound);
                    }
                }

                

                
            }
        }
       

        if(g_recoilAnim > 0){
            g_recoilAnim = g_recoilAnim - 3 * delta_t;
        } else {
            g_recoilAnim = 0;
        }

        if(g_ak_downtime > 0){
            g_ak_downtime = g_ak_downtime - delta_t;
        } else {
            g_ak_downtime = 0;
        }


        // Makeshift crosshair
        DrawSphere(camera_position_c + camera_view_vector * 0.2f, 0.001f, 0);


        bool is_shooting = false;

        if(g_LeftMouseButtonPressed && (gunType == 1)){
            if(g_ak_downtime <= 0){
                g_ak_downtime = g_ak_downtime_const;
                g_recoilAnim = 1;
                is_shooting = true;
            }
        }


        if(g_TapFlag && gunType == 0){
            g_TapFlag = false;
            g_recoilAnim = 1;
            is_shooting = true;
        }

        if(is_shooting){
            ma_sound_stop(&gunshot_sound);
            ma_sound_seek_to_pcm_frame(&gunshot_sound, 0);
            ma_sound_start(&gunshot_sound);
            
            float min_dist = -1.0f;

            glm::vec4 rayCastPointClosest;
            PhysicsObject * rayCastSelectedObjectPointer;
            for(PhysicsObject &object : PhysicsObjects){
                rayCastPoint = p_collision_sphere_ray(object.position, object.radius, camera_position_c, camera_view_vector, &rayCastDist);

                if(((rayCastDist < min_dist) && (rayCastDist >= 0)) ||( min_dist > -1.1f && min_dist < -0.9f )){
                    min_dist = rayCastDist;       
                    rayCastPointClosest = rayCastPoint;
                    rayCastSelectedObjectPointer = &object;           
                }
            }

            if(min_dist >= 0.0f){ // testa se o raycast encontrou algum objeto
                DrawSphere(rayCastPointClosest, 0.03f, 0);
                glm::vec4 impactVector = -5.0f * normalize(rayCastPointClosest - rayCastSelectedObjectPointer->position);
                float multiplier;
                if(opening_shot){
                    multiplier = opening_multiplier;
                    opening_shot = false;
                } else if (gunType == 1){
                    multiplier = 3.0f;
                } else {
                    multiplier = 1.0f;  
                }

                rayCastSelectedObjectPointer->movement_vector = (0.5f * rayCastSelectedObjectPointer->movement_vector
                                                                         + 0.6f * impactVector
                                                                          + 0.4f * planarize(camera_view_vector) * multiplier);


                ma_sound_stop(&clack_sound);
                ma_sound_seek_to_pcm_frame(&clack_sound, 0);
                ma_sound_start(&clack_sound);   
            }
        } 

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    //encerra engine de som
    ma_engine_uninit(&engine);

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureTableTop"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TexturePoolTable"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureObjUnkown"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureCueBall"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall1"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall2"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall3"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall4"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall5"), 8);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall6"), 9);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall7"), 10);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall8"), 11);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall9"), 12);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall10"), 13);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall11"), 14);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall12"), 15);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall13"), 16);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall14"), 17);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureBall15"), 18);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "brick_room_texture"), 19);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "ak47_texture"), 20);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variávelw 
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
        g_TapFlag = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;
    float zoom_vision_slowdown = Bezier(1, 1, 0.3,  0.3, g_zoomAnim);
    if (!g_FreeCamera)
    {
        
        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx * zoom_vision_slowdown;
        g_CameraPhi   += 0.01f*dy * zoom_vision_slowdown;
    
        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;
    
        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;
    

    } else {

    
        // Atualizamos parâmetros da câmera com os deslocamentos
        g_free_CameraTheta -= 0.01f*dx * zoom_vision_slowdown;
        g_free_CameraPhi   += 0.01f*dy * zoom_vision_slowdown;
    
        if (g_free_CameraPhi > phimax)
            g_free_CameraPhi = phimax;
    
        if (g_free_CameraPhi < phimin)
            g_free_CameraPhi = phimin;


    }

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;

    if (g_RightMouseButtonPressed)
    {
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ====================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ====================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_Y && action == GLFW_RELEASE)
    {
        resetBalls();
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }


    // Guns
    // Se o usuário apertar a tecla 1, muda pra ak47
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        gunType = 1;
    }

    // Se o usuário apertar a tecla 1, muda pra pistola
    if (key == GLFW_KEY_0 && action == GLFW_PRESS)
    {
        gunType = 0;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        g_FreeCamera = !g_FreeCamera;   
    
    }
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
        w_held = true;
    }
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
        a_held = true;
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        s_held = true;
    }
    if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        d_held = true;
    }
    if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
    {
        shift_held = true;
    }
    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS)
    {
        ctrl_held = true;
    }
    //release
    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
    {
        w_held = false;
    }
    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    {
        a_held = false;
    }
    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
    {
        s_held = false;
    }
    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    {
        d_held = false;
    }
    if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
    {
        shift_held = false;
    }
    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE)
    {
        ctrl_held = false;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

void DrawSphere(glm::vec4 position, float radius, int index){
    glm::mat4 model = Matrix_Translate(position.x, position.y, position.z) 
        * Matrix_Scale(radius, radius, radius);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, index + 10);
    DrawVirtualObject("the_sphere");

}


void DrawSphereCoords(int x, int y, int z, float radius){
    glm::mat4 model = Matrix_Translate(x,y,z) 
        * Matrix_Scale(radius, radius, radius);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, SPHERE);
    DrawVirtualObject("the_sphere");

}

float ellapsed_time(){
    static float old_seconds = (float)glfwGetTime();
    float seconds = (float)glfwGetTime();
    return seconds - old_seconds; 

}

//==========================================================================||
//||                                                                        ||
//||                  Collisions                                            ||
//||                                                                        ||
//==========================================================================||


float t_colision_sphere_plane(PhysicsObject sphere, char axis, int direction, float offset){
    
    float dt = -1;
    if(direction == 1){
        if(axis == 'x'){
            dt = (offset - sphere.position.x + sphere.radius) / sphere.movement_vector.x;
        }
        if(axis == 'y'){
            dt = (offset - sphere.position.y + sphere.radius) / sphere.movement_vector.y;        
            }
        if(axis == 'z'){
            dt = (offset - sphere.position.z + sphere.radius) / sphere.movement_vector.z;        
            }
    } else {
        if(axis == 'x'){
            dt = (offset - sphere.position.x - sphere.radius) / sphere.movement_vector.x;        
            }
        if(axis == 'y'){
            dt = (offset - sphere.position.y - sphere.radius) / sphere.movement_vector.y;        
            }
        if(axis == 'z'){
            dt = (offset - sphere.position.z - sphere.radius) / sphere.movement_vector.z;        
            }
    }
    
    
    return dt;
}


float distance(PhysicsObject o1, PhysicsObject o2){

    if(o1.type == "ball" && o2.type == "ball"){

        return sqrt(pow((o1.position.x - o2.position.x), 2) + pow((o1.position.y - o2.position.y), 2) + pow((o1.position.z - o2.position.z), 2));
    }
    return -1;
}

float dist(glm::vec4 v1, glm::vec4 v2){


    return sqrt(pow((v1.x - v2.x), 2) + pow((v1.y - v2.y), 2) + pow((v1.z - v2.z), 2));

}


glm::vec4 getVectorBetween(PhysicsObject o1, PhysicsObject o2){

    return o2.position - o1.position;
}

glm::vec4 planarize(glm::vec4 v){

    glm::vec4 planar = v;
    planar.y = 0.0f;
    return planar;

}

bool collideSpheres(PhysicsObject * o1, PhysicsObject * o2){

    float inset = (o1->radius + o2->radius) - distance(*o1, *o2);
    if(inset > 0){
        // Collision detected, now deal with new vectors

        glm::vec4 v1 = o1->movement_vector;
        glm::vec4 v2 = o2->movement_vector;
        glm::vec4 x1 = o1->position;
        glm::vec4 x2 = o2->position;
        float m1 = o1->mass;
        float m2 = o2->mass;

        glm::vec4 v1_between = (x1 - x2);
        glm::vec4 v2_between = (x2 - x1);

        float v1_scalar = ((2 * m2) / (m1 + m2)) * (dotproduct(v1 - v2,  v1_between) / pow(length(v1_between), 2));
        float v2_scalar = ((2 * m1) / (m1 + m2)) * (dotproduct(v2 - v1,  v2_between) / pow(length(v2_between), 2));

        glm::vec4 new_v1;
        glm::vec4 new_v2;
        
        if(v1_scalar == 0 )
        {
            new_v1 = v1;
        } else {
            new_v1 = v1 - v1_between * v1_scalar;
        }
        if(v2_scalar == 0)
        {
            new_v2 = v2;
        } else {
            new_v2 = v2 - v2_between * v2_scalar;
        }

        o1->movement_vector = new_v1;
        o2->movement_vector = new_v2;

        glm::vec4 o1_out_vector = normalize(o1->position - o2->position);
        o1->position = o1->position + o1_out_vector * (inset /2);
        o2->position = o2->position + o1_out_vector * (- inset /2);
        return true;
    } 
    return false;
}

glm::vec4 LERP(glm::vec4 p1, glm::vec4 p2, float t){

    float x = p1.x * (1 - t) + p2.x * t;
    float y = p1.y * (1 - t) + p2.y * t;
    float z = p1.z * (1 - t) + p2.z * t;
    float type = p1.w;

    return glm::vec4(x, y, z, type);

} 

float LERP(float f1, float f2, float t){

    float x = f1 * (1 - t) + f2 * t;
    return x;

}   

glm::vec4 Bezier(glm::vec4 p1, glm::vec4 p2, glm::vec4 p3, glm::vec4 p4, float t){

    glm::vec4 a = LERP(p1, p2, t);
    glm::vec4 b = LERP(p2, p3, t);
    glm::vec4 c = LERP(p3, p4, t);

    glm::vec4 d = LERP(a, b, t);
    glm::vec4 e = LERP(b, c, t);

    glm::vec4 f = LERP(d, e, t);

    return f;
} 

float Bezier(float f1, float f2, float f3, float f4, float t){

    float a = LERP(f1, f2, t);
    float b = LERP(f2, f3, t);
    float c = LERP(f3, f4, t);

    float d = LERP(a, b, t);
    float e = LERP(b, c, t);

    float f = LERP(d, e, t);

    return f;
} 


void resetBalls(){

    float x = -0.3f;
    float y = yMinusBound;
    float z = (zPlusBound + zMinusBound) /2;
    float diameter = g_ball_radius * 2 + 0.01f;
    

    PhysicsObjects.clear();
    global_Object_Index = 0;
    opening_shot = true;

    PhysicsObject b1 = PhysicsObject("ball", g_ball_radius, 30.0f, 0.7f, 0.0, z);
    b1.setMovementVector(0.0f, 0.1f, 0.0f);
    PhysicsObjects.push_back(b1);

    for(int i = 0; i <5; i++){
        for(int j = 0; j < i; j++){

            for(int k = 0; k < 1; k++){
                PhysicsObject b = PhysicsObject("ball", g_ball_radius, 30.0f, x, k * y, z);
                b.setMovementVector(0.0f, 0.1f, 0.0f);
                PhysicsObjects.push_back(b);

            }

            z = z + diameter ;
        }
         z = z - (diameter * (i + 0.5f));
        x = x - diameter * sqrt(3) / 2;
    }

}


// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :