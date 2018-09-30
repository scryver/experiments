#define MATRIX_MAP(name) f32 name(f32 a, void *user)
typedef MATRIX_MAP(MatrixMap);
#define MATRIX_MAP_AT(name) f32 name(u32 row, u32 column, f32 a, void *user)
typedef MATRIX_MAP_AT(MatrixMapAt);

internal void
matrix_add_scalar(u32 rows, u32 columns, f32 *matrix, f32 scalar)
{
    for (u32 row = 0; row < rows; ++row)
    {
        f32 *r = matrix + row * columns;
        for (u32 col = 0; col < columns; ++col)
        {
            r[col] += scalar;
        }
    }
}

internal void
matrix_multiply_scalar(u32 rows, u32 columns, f32 *matrix, f32 scalar)
{
    for (u32 row = 0; row < rows; ++row)
    {
        f32 *r = matrix + row * columns;
        for (u32 col = 0; col < columns; ++col)
        {
            r[col] *= scalar;
        }
    }
}

internal void
matrix_add_matrix(u32 rows, u32 columns, f32 *add, f32 *to)
{
    for (u32 row = 0; row < rows; ++row)
    {
        f32 *rA = add + row * columns;
        f32 *rT = to + row * columns;
        for (u32 col = 0; col < columns; ++col)
        {
            rT[col] += rA[col];
        }
    }
}

internal void
matrix_sub_matrix(u32 rows, u32 columns, f32 *sub, f32 *from)
{
    for (u32 row = 0; row < rows; ++row)
    {
        f32 *rS = sub + row * columns;
        f32 *rF = from + row * columns;
        for (u32 col = 0; col < columns; ++col)
        {
            rF[col] -= rS[col];
        }
    }
}

internal void
matrix_hadamard_matrix(u32 rows, u32 columns, f32 *factor, f32 *dest)
{
    for (u32 row = 0; row < rows; ++row)
    {
        f32 *rF = factor + row * columns;
        f32 *rD = dest + row * columns;
        for (u32 col = 0; col < columns; ++col)
        {
            rD[col] *= rF[col];
        }
    }
}

internal void
matrix_multiply_matrix(u32 aRows, u32 aCols_bRows, u32 bColumns, f32 *a, f32 *b, f32 *result)
{
    for (u32 row = 0; row < aRows; ++row)
    {
        f32 *rowA = a + row * aCols_bRows;
        f32 *rowR = result + row * bColumns;
        for (u32 col = 0; col < bColumns; ++col)
        {
            f32 *colB = b + col;
             rowR[col] = 0.0f;
            for (u32 s = 0; s < aCols_bRows; ++s)
        {
                rowR[col] += rowA[s] * colB[s * bColumns];
        }
        }
    }
}

internal void
matrix_copy(u32 rows, u32 columns, f32 *from, f32 *to)
{
    for (u32 row = 0; row < rows; ++row)
    {
        f32 *rF = from + row * columns;
        f32 *rT = to + row * columns;
        for (u32 col = 0; col < columns; ++col)
        {
            rT[col] = rF[col];
        }
    }
}

internal void
matrix_transpose(u32 rows, u32 columns, f32 *from, f32 *to)
{
    for (u32 col = 0; col < columns; ++col)
        {
        f32 *rF = from + col;
        f32 *rT = to + col * rows;
        for (u32 row = 0; row < rows; ++row)
    {
            rT[row] = rF[row * columns];
        }
    }
}

internal void
matrix_map(u32 rows, u32 columns, f32 *m, MatrixMap *map, void *user = 0)
{
    for (u32 row = 0; row < rows; ++row)
    {
        f32 *r = m + row * columns;
        for (u32 col = 0; col < columns; ++col)
        {
            r[col] = map(r[col], user);
        }
    }
}

internal void
matrix_map(u32 rows, u32 columns, f32 *m, MatrixMapAt *map, void *user = 0)
{
    for (u32 row = 0; row < rows; ++row)
    {
        f32 *r = m + row * columns;
        for (u32 col = 0; col < columns; ++col)
        {
            r[col] = map(row, col, r[col], user);
        }
    }
}
