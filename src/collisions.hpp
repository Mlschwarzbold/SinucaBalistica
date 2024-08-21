#ifndef _COLLISIONS_H
#define _COLLISIONS_H

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

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"



float dist2(glm::vec4 v1, glm::vec4 v2){


    return sqrt(pow((v1.x - v2.x), 2) + pow((v1.y - v2.y), 2) + pow((v1.z - v2.z), 2));

}


glm::vec4 planarize2(glm::vec4 v){

    glm::vec4 planar = v;
    planar.y = 0.0f;
    return planar;

}


// Point - cillinder detection
bool point_cillinder_collide(glm::vec4 point, glm::vec4 cillinder_coords, float width){
    return dist2(planarize2(point) , planarize2(cillinder_coords)) < width;
}

bool collision_box_box(glm::vec4 b1_coords, glm::vec4 b2_coords, glm::vec4 b1_dim, glm::vec4 b2_dim){

    //glm::vec4 push_v = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);


    if( (b1_coords.x - b1_dim.x) > (b2_coords.x + b2_dim.x)){
        return false;
    }
    if( (b1_coords.x + b1_dim.x) < (b2_coords.x - b2_dim.x)){
        return false;
    }
    if( (b1_coords.z - b1_dim.z) > (b2_coords.z + b2_dim.z)){
        return false;
    }
    if( (b1_coords.z + b1_dim.z) < (b2_coords.z - b2_dim.z)){
        return false;
    }

    return true;
}



// sphere - ray collision
glm::vec4 p_collision_sphere_ray(glm::vec4 spherePos, float radius, glm::vec4 rayPos, glm::vec4 rayVec, float * dist){

    glm::vec4 rayPosCentered = rayPos - spherePos;


    float a = (pow(rayVec.x, 2) + pow(rayVec.y, 2) + pow(rayVec.z, 2));
    float b = (2 * (rayPosCentered.x * rayVec.x + rayPosCentered.y * rayVec.y + rayPosCentered.z * rayVec.z));
    float c = pow((rayPosCentered.x), 2) +  pow((rayPosCentered.y), 2) +  pow((rayPosCentered.z), 2)  - pow(radius,2);

    float delta = pow(b, 2) - 4 * a * c;

    float dt;
    glm::vec4 contact;

    if(delta >= 0){
        dt = (-b - sqrt(delta)) / (2 *a);
        contact = rayPos + rayVec * dt;
    } else {
        dt = -1;
        contact = glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
    }
    
    *dist = dt * norm(rayVec);
    return contact;
}



#endif // _COLLISIONS_H
// vim: set spell spelllang=pt_br :
