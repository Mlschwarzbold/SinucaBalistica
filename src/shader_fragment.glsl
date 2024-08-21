#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

in float lamber_gourad;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define UNKNOWN -2
#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define GUN 3
#define TABLE_TOP 4
#define BRICK_ROOM 21
#define AK47 26
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureCueBall;
uniform sampler2D TextureBall1;
uniform sampler2D TextureBall2;
uniform sampler2D TextureBall3;
uniform sampler2D TextureBall4;
uniform sampler2D TextureBall5;
uniform sampler2D TextureBall6;
uniform sampler2D TextureBall7;
uniform sampler2D TextureBall8;
uniform sampler2D TextureBall9;
uniform sampler2D TextureBall10;
uniform sampler2D TextureBall11;
uniform sampler2D TextureBall12;
uniform sampler2D TextureBall13;
uniform sampler2D TextureBall14;
uniform sampler2D TextureBall15;
uniform sampler2D brick_room_texture;
uniform sampler2D ak47_texture;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,12.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = - l + 2 * n * (dot(n, l)); 

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    float q; // Expoente especular para o modelo de iluminação de Phong

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    if ( object_id == SPHERE )
    {
        // PREENCHA AQUI as coordenadas de textura da esfera, computadas com
        // projeção esférica EM COORDENADAS DO MODELO. Utilize como referência
        // o slides 134-150 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // A esfera que define a projeção deve estar centrada na posição
        // "bbox_center" definida abaixo.

        // Você deve utilizar:
        //   função 'length( )' : comprimento Euclidiano de um vetor
        //   função 'atan( , )' : arcotangente. Veja https://en.wikipedia.org/wiki/Atan2.
        //   função 'asin( )'   : seno inverso.
        //   constante M_PI
        //   variável position_model

        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;
        vec4 p_vector = position_model - bbox_center;

        float px = p_vector.x;
        float py = p_vector.y;
        float pz = p_vector.z;

        float rho = length(p_vector);
        float theta = atan(px, pz);
        float phi = asin(py / rho);

        U = (theta + M_PI) / (2 * M_PI);
        V = (phi + M_PI_2) / M_PI;

        // Propriedades espectrais da esfera
        Kd = vec3(0.8,0.4,0.08);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.4,0.2,0.04);
        q = 1.0;
    }
    else if ( object_id == BUNNY)
    {
        // PREENCHA AQUI as coordenadas de textura do coelho, computadas com
        // projeção planar XY em COORDENADAS DO MODELO. Utilize como referência
        // o slides 99-104 do documento Aula_20_Mapeamento_de_Texturas.pdf,
        // e também use as variáveis min*/max* definidas abaixo para normalizar
        // as coordenadas de textura U e V dentro do intervalo [0,1]. Para
        // tanto, veja por exemplo o mapeamento da variável 'p_v' utilizando
        // 'h' no slides 158-160 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // Veja também a Questão 4 do Questionário 4 no Moodle.

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - minx) / (maxx - minx);
        V = (position_model.y - miny) / (maxy - miny);
    }
    else if ( object_id == PLANE || object_id == TABLE_TOP || object_id == GUN || object_id == BRICK_ROOM)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        // Propriedades espectrais da mesa
        Kd = vec3(0.2,0.2,0.2);
        Ks = vec3(0.3,0.3,0.3);
        Ka = vec3(0.0,0.0,0.0);
        q = 20.0;
    } 
    else if ( object_id == GUN || object_id == AK47)
    {
        // PREENCHA AQUI as coordenadas de textura do coelho, computadas com
        // projeção planar XY em COORDENADAS DO MODELO. Utilize como referência
        // o slides 99-104 do documento Aula_20_Mapeamento_de_Texturas.pdf,
        // e também use as variáveis min*/max* definidas abaixo para normalizar
        // as coordenadas de textura U e V dentro do intervalo [0,1]. Para
        // tanto, veja por exemplo o mapeamento da variável 'p_v' utilizando
        // 'h' no slides 158-160 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // Veja também a Questão 4 do Questionário 4 no Moodle.

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - minx) / (maxx - minx);
        V = (position_model.y - miny) / (maxy - miny);

        // Propriedades espectrais da arma
        Kd = vec3(0.08,0.4,0.8);
        Ks = vec3(0.8,0.8,0.8);
        Ka = Kd/2; //vec3(0.0,0.0,0.0);
        q = 32.0;
    } 
    else if( object_id >= 10 && object_id <= 25 )
    {
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;
        vec4 p_vector = position_model - bbox_center;

        float px = p_vector.x;
        float py = p_vector.y;
        float pz = p_vector.z;

        float rho = length(p_vector);
        float theta = atan(px, pz);
        float phi = asin(py / rho);

        U = (theta + M_PI) / (2 * M_PI);
        V = (phi + M_PI_2) / M_PI;

        // Propriedades espectrais da esfera
        Kd = vec3(0.8,0.4,0.08);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.4,0.2,0.04);
        q = 1.0;
    }
    else
    {
        U = texcoords.x;
        V = texcoords.y;
        
        Kd = vec3(0.0,0.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.0,0.0,0.0);
        q = 1.0;
    }

    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    vec3 Kd0;
    vec3 Kd1;
    if ( object_id == GUN )
    {
        Kd0 = texture(TextureImage2, vec2(U,V)).rgb;
        Kd1 = texture(TextureImage2, vec2(U,V)).rgb;
    } else if ( object_id == TABLE_TOP ){
        Kd0 = texture(TextureImage3, vec2(U,V)).rgb;
        Kd1 = texture(TextureImage3, vec2(U,V)).rgb;
    } else if ( object_id == UNKNOWN ){
        Kd0 = texture(TextureImage4, vec2(U,V)).rgb;
        Kd1 = texture(TextureImage4, vec2(U,V)).rgb;
    } else if ( object_id == BRICK_ROOM ){
        Kd0 = texture(brick_room_texture, vec2(U,V)).rgb;
        Kd1 = texture(brick_room_texture, vec2(U,V)).rgb;
    } else if (object_id == 10 ){
        Kd0 = texture(TextureCueBall, vec2(U,V)).rgb;
        Kd1 = texture(TextureCueBall, vec2(U,V)).rgb;
    } else if (object_id == 11 ){
        Kd0 = texture(TextureBall1, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall1, vec2(U,V)).rgb;
    } else if (object_id == 12 ){
        Kd0 = texture(TextureBall2, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall2, vec2(U,V)).rgb;
    } else if (object_id == 13 ){
        Kd0 = texture(TextureBall3, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall3, vec2(U,V)).rgb;
    } else if (object_id == 14 ){
        Kd0 = texture(TextureBall4, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall4, vec2(U,V)).rgb;
    } else if (object_id == 15 ){
        Kd0 = texture(TextureBall5, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall5, vec2(U,V)).rgb;
    } else if (object_id == 16 ){
        Kd0 = texture(TextureBall6, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall6, vec2(U,V)).rgb;
    } else if (object_id == 17 ){
        Kd0 = texture(TextureBall7, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall7, vec2(U,V)).rgb;
    } else if (object_id == 18 ){
        Kd0 = texture(TextureBall8, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall8, vec2(U,V)).rgb;
    } else if (object_id == 19 ){
        Kd0 = texture(TextureBall9, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall9, vec2(U,V)).rgb;
    } else if (object_id == 20 ){
        Kd0 = texture(TextureBall10, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall10, vec2(U,V)).rgb;
    } else if (object_id == 21 ){
        Kd0 = texture(TextureBall11, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall11, vec2(U,V)).rgb;
    } else if (object_id == 22 ){
        Kd0 = texture(TextureBall12, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall12, vec2(U,V)).rgb;
    } else if (object_id == 23 ){
        Kd0 = texture(TextureBall13, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall13, vec2(U,V)).rgb;
    } else if (object_id == 24 ){
        Kd0 = texture(TextureBall14, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall14, vec2(U,V)).rgb;
    } else if (object_id == 25 ){
        Kd0 = texture(TextureBall15, vec2(U,V)).rgb;
        Kd1 = texture(TextureBall15, vec2(U,V)).rgb;
    } else if ( object_id == AK47 ){
        Kd0 = texture(ak47_texture, vec2(U,V)).rgb;
        Kd1 = texture(ak47_texture, vec2(U,V)).rgb;
    } else {
        Kd0 = texture(brick_room_texture, vec2(U,V)).rgb;
        Kd1 = texture(brick_room_texture, vec2(U,V)).rgb;
    }
    
    // Espectro da fonte de iluminação
    vec3 I = vec3(1.0,1.0,1.0); 

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.2,0.2,0.2);

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec3 lambert_diffuse_term = Kd * I * max(0 , dot(n,l));

    // Termo ambiente
    vec3 ambient_term = Ka * Ia;

    // Termo especular utilizando o modelo de iluminação de Phong
    vec3 phong_specular_term  = Ks * I * pow(max(0,dot(r, v)), q);
   

    if ( object_id == BRICK_ROOM) {
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        color.rgb = Kd0 * (pow(lambert,1) + 0.01) + Kd1 * (1 - (pow(lambert, 0.2)) + 0.01);
    } else if ( object_id >= 10 && object_id <= 25 ) {
        color.rgb = Kd0 * (pow(lamber_gourad,1) + 0.01) + Kd1 * (1 - (pow(lamber_gourad, 0.2)) + 0.01);
    }
    else {
        color.rgb = lambert_diffuse_term + ambient_term + phong_specular_term;
        color.rgb = Kd0 * color.rgb;
    }

    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = 1;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
} 

