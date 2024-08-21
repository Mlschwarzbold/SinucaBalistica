#ifndef _PHYSICSOBJECT_H
#define _PHYSICSOBJECT_H

#include <cstdio>
#include <cstdlib>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "matrices.h"
#include "globals.hpp"
#include "utils.h"
#include "renderFunctions.hpp"


class PhysicsObject;

float distance(PhysicsObject o1, PhysicsObject o2);
float dist(glm::vec4 v1, glm::vec4 v2);
// Collisions
float t_colision_sphere_plane(PhysicsObject sphere, char axis, int direction, float offset);
void collideSpheres(PhysicsObject * o1, PhysicsObject * o2);
glm::vec4 p_collision_sphere_ray(glm::vec4 spherePos, float radius, glm::vec4 rayPos, glm::vec4 rayVecd, float * dist);
glm::vec4 getVectorBetween(PhysicsObject o1, PhysicsObject o2);
glm::vec4 planarize(glm::vec4 v);

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
        //DrawSphere(position, radius, index);
            glm::mat4 model = Matrix_Translate(position.x, position.y, position.z) 
                * Matrix_Scale(radius, radius, radius)
                * rotation_matrix;
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, index + 10);
            DrawVirtualObject("the_sphere");
            
    }

    void advance_time(float dt){
        
        // Update position
        position = position + movement_vector * dt;

        // Update angle

        

        if(glm::length(planarize(movement_vector)) > 0.1 /*&& pow(movement_vector.x,2 ) > 0.1f && pow(movement_vector.z, 2) > 0.1f */){
            
            //rotateByAxis(up_vector, 3.14/6);
            glm::vec4 rotation_axis = normalize(crossproduct(up_vector, movement_vector));
            float angle = 1 * length(movement_vector); // math
            //std::cout << length(rotation_axis) << std::endl;
            
            glm::vec4 weird_vector = normalize(glm::vec4(1.0f, 2.0f, 1.0f, 0.0f));

            rotateByAxis(weird_vector, length(movement_vector) * dt * 10.0f);        
        }
    }

    

    void reflectAxis(char axis){
        float directional_loss = 0.5;
        if(axis == 'x'){
            movement_vector.x = -movement_vector.x * directional_loss;
            //encacapar(glm::vec4(0.0f,0.0f,0.0f,1.0f));
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


    void collideNormal(glm::vec4 point, glm::vec4 vector){
        glm::vec4 normal = normalize(vector);
        
        
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
        //bool inHole = collideWithHole();


        
        if((position.y - radius < tableHeight)){
            position.y = position.y + (tableHeight - position.y + radius);
            reflectAxis('y');
        }

    }

    
    bool collideWithHole(glm::vec4 hole, float tableHeight){

        // Collide with the bottom of the hole
        float holeBottomY = tableHeight - 0.5f;
        if(position.y - radius < holeBottomY){
            position.y = position.y + (holeBottomY - position.y + radius);
            reflectAxis('y');
        }

        if(dist(planarize(position) , planarize(hole)) < hole_width){

            
            if( dist(planarize(position) , planarize(hole))< (hole_width - radius)){
                //printf("No collisions\n");
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
            printf("Ball inside hole\n");
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

void collideSpheres(PhysicsObject * o1, PhysicsObject * o2){

    float inset = (o1->radius + o2->radius) - distance(*o1, *o2);
    if(inset > 0){
        // Collision detected, now deal with new vectors

        //std::cout << "COLLLIIIIIIIIIIIIIIIISION" << std::endl;

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

        //o1->advance_time(0.001);
        //o2->advance_time(0.001);

        glm::vec4 o1_out_vector = normalize(o1->position - o2->position);
        o1->position = o1->position + o1_out_vector * (inset /2);
        o2->position = o2->position + o1_out_vector * (- inset /2);
    } 
}

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

float t_colision_sphere_plane(PhysicsObject sphere, char axis, int direction, float offset){
    
    float dt = -1;
    if(direction == 1){
        if(axis = 'x'){
            dt = (offset - sphere.position.x + sphere.radius) / sphere.movement_vector.x;
        }
        if(axis = 'y'){
            dt = (offset - sphere.position.y + sphere.radius) / sphere.movement_vector.y;        
            }
        if(axis = 'z'){
            dt = (offset - sphere.position.z + sphere.radius) / sphere.movement_vector.z;        
            }
    } else {
        if(axis = 'x'){
            dt = (offset - sphere.position.x - sphere.radius) / sphere.movement_vector.x;        
            }
        if(axis = 'y'){
            dt = (offset - sphere.position.y - sphere.radius) / sphere.movement_vector.y;        
            }
        if(axis = 'z'){
            dt = (offset - sphere.position.z - sphere.radius) / sphere.movement_vector.z;        
            }
    }
    
    
    return dt;
}


#endif // _PHYSICSOBJECT_H
// vim: set spell spelllang=pt_br :
