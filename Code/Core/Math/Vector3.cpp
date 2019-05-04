#include "Vector3.h"
#include "Vector.h"

namespace lf {
const Vector3 Vector3::One = Vector3(1.0f, 1.0f, 1.0f);
const Vector3 Vector3::Zero = Vector3(0.0f, 0.0f, 0.0f);
const Vector3 Vector3::Up = Vector3(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::Right = Vector3(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::Forward = Vector3(0.0f, 0.0f, 1.0f); 

const Vector Vector::Forward = Vector(0.0f, 0.0f, 1.0f, 1.0f);
const Vector Vector::Up = Vector(0.0f, 1.0f, 0.0f, 1.0f);
const Vector Vector::Right = Vector(1.0f, 0.0f, 0.0f, 1.0f);
const Vector Vector::Zero = Vector(0.0f, 0.0f, 0.0f, 0.0f);
const Vector Vector::One = Vector(1.0f, 1.0f, 1.0f, 1.0f);
} // namespace lf