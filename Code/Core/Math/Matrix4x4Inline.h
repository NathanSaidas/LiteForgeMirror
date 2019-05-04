namespace lf
{
    Matrix4x4::Matrix4x4()
    {
        memset(m, 0, sizeof(m));
    }
    Matrix4x4::Matrix4x4(const Matrix4x4& other)
    {
        memcpy(m, other.m, sizeof(m));
    }
    Matrix4x4::Matrix4x4(const Matrix& other)
    {
        other.GetAll(m);
        m[0][3] = m[3][0]; m[3][0] = 0.0f;
        m[1][3] = m[3][1]; m[3][1] = 0.0f;
        m[2][3] = m[3][2]; m[3][2] = 0.0f;
    }
}