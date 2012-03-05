// Transform header file to define the interface. 
// The class is all static for simplicity
// You need to implement left, up and lookAt
// Rotate is a helper function

// Include the helper glm library, including matrix transform extensions

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// glm provides vector, matrix classes like glsl
// Typedefs to make code more readable 

typedef glm::mat3 mat3 ;
typedef glm::mat4 mat4 ; 
typedef glm::vec3 vec3 ; 
typedef glm::vec4 vec4 ; 
const float pi = 3.14159265 ; // For portability across platforms


class Transform  
{
public:
	Transform();
	virtual ~Transform();
	static void left(float degrees, vec3& eye, vec3& up);
	// MICE
	static void centerleft(float degrees, vec3& eye, vec3& center, vec3& up) ;
	static void centerup(float degrees, vec3& eye, vec3& center, vec3& up);
	static void zoom(float dist, vec3& eye, vec3& center);
	// MICE
	// CONTROL
	static void centeralign(vec3 eye, vec3 move_center, vec3& center, vec3& up, float speed);
	static void moveleft(float degrees, vec3 eye, vec3& move_center);
	// CONTROL
	static void up(float degrees, vec3& eye, vec3& center, vec3& up);
	static mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up);
	static mat4 perspective(float fovy, float aspect, float zNear, float zFar);
    static mat3 rotate(const float degrees, const vec3& axis) ;
    static mat4 scale(const float &sx, const float &sy, const float &sz) ; 
    static mat4 translate(const float &tx, const float &ty, const float &tz);
};

