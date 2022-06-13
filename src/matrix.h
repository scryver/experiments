#define MATRIX_MAP(name) f32 name(f32 a, void *user)
typedef MATRIX_MAP(MatrixMap);
#define MATRIX_MAP_AT(name) f32 name(u32 row, u32 column, f32 a, void *user)
typedef MATRIX_MAP_AT(MatrixMapAt);

#ifndef MATRIX_TEST
#define MATRIX_TEST 0
#endif

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

#if MATRIX_TEST

internal inline b32
float_almost_equal(f32 a, f32 b, f32 eps = 1e-12f)
{
    b32 result = false;
    if (((a - eps) <= b) &&
        ((a + eps) >= b))
    {
        result = true;
    }
return result;
}

#define START_TEST(name) \
internal void \
test_##name(void) \
{ \
    fprintf(stderr, "Starting test " #name "... ");

#define END_TEST() \
    fprintf(stderr, "DONE\n"); \
}

START_TEST(matrix_add_scalar)
{
    f32 matrix[16] = {};
    matrix[0] = 1.0f;
    matrix[1] = 2.0f;
    matrix[2] = 3.0f;
    matrix[3] = 4.0f;
    u32 rows = 4;
    u32 columns = 4;
    matrix_add_scalar(rows, columns, matrix, 5.0f);

    i_expect(float_almost_equal(matrix[0], 6.0f));
    i_expect(float_almost_equal(matrix[1], 7.0f));
    i_expect(float_almost_equal(matrix[2], 8.0f));
    i_expect(float_almost_equal(matrix[3], 9.0f));
    i_expect(float_almost_equal(matrix[4], 5.0f));
    i_expect(float_almost_equal(matrix[10], 5.0f));
    i_expect(float_almost_equal(matrix[15], 5.0f));
}
END_TEST()

START_TEST(matrix_multiply_scalar)
{
    f32 matrix[16] = {};
    matrix[0] = 1.0f;
    matrix[1] = 2.0f;
    matrix[2] = 3.0f;
    matrix[3] = 4.0f;
    u32 rows = 4;
    u32 columns = 4;
    matrix_multiply_scalar(rows, columns, matrix, 5.0f);
    i_expect(float_almost_equal(matrix[0], 5.0f));

    i_expect(float_almost_equal(matrix[0], 5.0f));
    i_expect(float_almost_equal(matrix[1], 10.0f));
    i_expect(float_almost_equal(matrix[2], 15.0f));
    i_expect(float_almost_equal(matrix[3], 20.0f));
    i_expect(float_almost_equal(matrix[4], 0.0f));
    i_expect(float_almost_equal(matrix[10], 0.0f));
    i_expect(float_almost_equal(matrix[15], 0.0f));
}
END_TEST()

START_TEST(matrix_add_matrix)
{
    f32 matrixA[16] = {};
    f32 matrixB[16] = {};
    matrixA[0] = 1.0f;
    matrixA[1] = 2.0f;
    matrixA[2] = 3.0f;
    matrixA[3] = 4.0f;
    matrixB[0] = 1.0f;
    matrixB[5] = 1.0f;
    matrixB[10] = 1.0f;
    matrixB[15] = 1.0f;
    u32 rows = 4;
    u32 columns = 4;
    matrix_add_matrix(rows, columns, matrixA, matrixB);

    // NOTE(michiel): A didn't change so we expect the floats to be truely equal
    i_expect(matrixA[0] == 1.0f);
    i_expect(matrixA[1] == 2.0f);
    i_expect(matrixA[2] == 3.0f);
    i_expect(matrixA[3] == 4.0f);
    i_expect(matrixA[4] == 0.0f);
    i_expect(matrixA[7] == 0.0f);
    i_expect(matrixA[10] == 0.0f);
    i_expect(matrixA[15] == 0.0f);
    i_expect(float_almost_equal(matrixB[0], 2.0f));
    i_expect(float_almost_equal(matrixB[1], 2.0f));
    i_expect(float_almost_equal(matrixB[2], 3.0f));
    i_expect(float_almost_equal(matrixB[3], 4.0f));
    i_expect(float_almost_equal(matrixB[4], 0.0f));
    i_expect(float_almost_equal(matrixB[5], 1.0f));
    i_expect(float_almost_equal(matrixB[6], 0.0f));
    i_expect(float_almost_equal(matrixB[10], 1.0f));
    i_expect(float_almost_equal(matrixB[15], 1.0f));
}
END_TEST()

START_TEST(matrix_sub_matrix)
{
    f32 matrixA[16] = {};
    f32 matrixB[16] = {};
    matrixA[0] = 1.0f;
    matrixA[1] = 2.0f;
    matrixA[2] = 3.0f;
    matrixA[3] = 4.0f;
    matrixB[0] = 1.0f;
    matrixB[5] = 1.0f;
    matrixB[10] = 1.0f;
    matrixB[15] = 1.0f;
    u32 rows = 4;
    u32 columns = 4;
    matrix_sub_matrix(rows, columns, matrixA, matrixB);

    // NOTE(michiel): A didn't change so we expect the floats to be truely equal
    i_expect(matrixA[0] == 1.0f);
    i_expect(matrixA[1] == 2.0f);
    i_expect(matrixA[2] == 3.0f);
    i_expect(matrixA[3] == 4.0f);
    i_expect(matrixA[4] == 0.0f);
    i_expect(matrixA[7] == 0.0f);
    i_expect(matrixA[10] == 0.0f);
    i_expect(matrixA[15] == 0.0f);
    i_expect(float_almost_equal(matrixB[0], 0.0f));
    i_expect(float_almost_equal(matrixB[1], -2.0f));
    i_expect(float_almost_equal(matrixB[2], -3.0f));
    i_expect(float_almost_equal(matrixB[3], -4.0f));
    i_expect(float_almost_equal(matrixB[4], 0.0f));
    i_expect(float_almost_equal(matrixB[5], 1.0f));
    i_expect(float_almost_equal(matrixB[6], 0.0f));
    i_expect(float_almost_equal(matrixB[10], 1.0f));
    i_expect(float_almost_equal(matrixB[15], 1.0f));
}
END_TEST()

START_TEST(matrix_hadamard_matrix)
{
    f32 matrixA[16] = {};
    f32 matrixB[16] = {};
    matrixA[0] = 1.0f;
    matrixA[1] = 2.0f;
    matrixA[2] = 3.0f;
    matrixA[3] = 4.0f;
    matrixB[0] = 1.0f;
    matrixB[5] = 1.0f;
    matrixB[10] = 1.0f;
    matrixB[15] = 1.0f;
    u32 rows = 4;
    u32 columns = 4;
    // (u32 rows, u32 columns, f32 *factor, f32 *dest)
    matrix_hadamard_matrix(rows, columns, matrixA, matrixB);

    // NOTE(michiel): A didn't change so we expect the floats to be truely equal
    i_expect(matrixA[0] == 1.0f);
    i_expect(matrixA[1] == 2.0f);
    i_expect(matrixA[2] == 3.0f);
    i_expect(matrixA[3] == 4.0f);
    i_expect(matrixA[4] == 0.0f);
    i_expect(matrixA[7] == 0.0f);
    i_expect(matrixA[10] == 0.0f);
    i_expect(matrixA[15] == 0.0f);
    i_expect(float_almost_equal(matrixB[0], 1.0f));
    i_expect(float_almost_equal(matrixB[1], 0.0f));
    i_expect(float_almost_equal(matrixB[2], 0.0f));
    i_expect(float_almost_equal(matrixB[3], 0.0f));
    i_expect(float_almost_equal(matrixB[4], 0.0f));
    i_expect(float_almost_equal(matrixB[5], 0.0f));
    i_expect(float_almost_equal(matrixB[6], 0.0f));
    i_expect(float_almost_equal(matrixB[10], 0.0f));
    i_expect(float_almost_equal(matrixB[15], 0.0f));
}
END_TEST()

START_TEST(matrix_multiply_matrix)
{
    f32 matrixA[12] = {}; // NOTE(michiel): 3rx4c
    f32 matrixB[8] = {};  // NOTE(michiel): 4rx2c

    f32 result[6] = {};   // NOTE(michiel): 3rx2c

    matrixA[0] = 1.0f;
    matrixA[1] = 2.0f;
    matrixA[2] = 3.0f;
    matrixA[3] = 4.0f;
    matrixA[4] = 3.0f;
    matrixA[5] = 4.0f;
    matrixA[6] = 1.0f;
    matrixA[7] = 2.0f;
    matrixA[8] = 1.0f;
    matrixA[9] = 4.0f;
    matrixA[10] = 2.0f;
    matrixA[11] = 3.0f;
    matrixB[0] = 1.0f;
    matrixB[1] = 2.0f;
    matrixB[2] = 2.0f;
    matrixB[3] = 4.0f;
    matrixB[4] = 3.0f;
    matrixB[5] = 6.0f;
    matrixB[6] = 4.0f;
    matrixB[7] = 8.0f;

    u32 rowsA = 3;
    u32 columnsA = 4;
    u32 columnsB = 2;
    // (u32 aRows, u32 aCols_bRows, u32 bColumns, f32 *a, f32 *b, f32 *result)
matrix_multiply_matrix(rowsA, columnsA, columnsB, matrixA, matrixB, result);

    // NOTE(michiel): A and B didn't change so we expect the floats to be truely equal

    i_expect(matrixA[0] == 1.0f);
    i_expect(matrixA[1] == 2.0f);
    i_expect(matrixA[2] == 3.0f);
    i_expect(matrixA[3] == 4.0f);
    i_expect(matrixA[4] == 3.0f);
    i_expect(matrixA[5] == 4.0f);
    i_expect(matrixA[6] == 1.0f);
    i_expect(matrixA[7] == 2.0f);
    i_expect(matrixA[8] == 1.0f);
    i_expect(matrixA[9] == 4.0f);
    i_expect(matrixA[10] == 2.0f);
    i_expect(matrixA[11] == 3.0f);
    i_expect(matrixB[0] == 1.0f);
    i_expect(matrixB[1] == 2.0f);
    i_expect(matrixB[2] == 2.0f);
    i_expect(matrixB[3] == 4.0f);
    i_expect(matrixB[4] == 3.0f);
    i_expect(matrixB[5] == 6.0f);
    i_expect(matrixB[6] == 4.0f);
    i_expect(matrixB[7] == 8.0f);
    // a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0]
    i_expect(float_almost_equal(result[0], 30.0f));
    // a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1]
    i_expect(float_almost_equal(result[1], 60.0f));
    // a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0]
    i_expect(float_almost_equal(result[2], 22.0f));
    // a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1]
    i_expect(float_almost_equal(result[3], 44.0f));
    // a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0]
    i_expect(float_almost_equal(result[4], 27.0f));
    // a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1]
    i_expect(float_almost_equal(result[5], 54.0f));
    }
END_TEST()

START_TEST(matrix_copy)
{
    f32 matrixA[8] = {}; // NOTE(michiel): 2rx4c
    f32 matrixB[8] = {};  // NOTE(michiel): 2rx4c

    matrixA[0] = 1.0f;
    matrixA[1] = 2.0f;
    matrixA[2] = 3.0f;
    matrixA[3] = 4.0f;
    matrixA[4] = 3.0f;
    matrixA[5] = 4.0f;
    matrixA[6] = 1.0f;
    matrixA[7] = 1.0f;
    u32 rows = 2;
    u32 columns = 4;
    matrix_copy(rows, columns, matrixA, matrixB);
    for (u32 r = 0; r < rows * columns; ++r)
    {
        i_expect(matrixA[r] == matrixB[r]);
    }
}
END_TEST()

START_TEST(matrix_transpose)
{
    f32 matrixA[8] = {}; // NOTE(michiel): 2rx4c
    f32 matrixB[8] = {};  // NOTE(michiel): 4rx2c

    matrixA[0] = 1.0f;
    matrixA[1] = 2.0f;
    matrixA[2] = 3.0f;
    matrixA[3] = 4.0f;
    matrixA[4] = 5.0f;
    matrixA[5] = 6.0f;
    matrixA[6] = 7.0f;
    matrixA[7] = 8.0f;
    u32 rows = 2;
    u32 columns = 4;
    matrix_transpose(rows, columns, matrixA, matrixB);
    i_expect(matrixA[0] == matrixB[0]);
    i_expect(matrixA[1] == matrixB[2]);
    i_expect(matrixA[2] == matrixB[4]);
    i_expect(matrixA[3] == matrixB[6]);
    i_expect(matrixA[4] == matrixB[1]);
    i_expect(matrixA[5] == matrixB[3]);
    i_expect(matrixA[6] == matrixB[5]);
    i_expect(matrixA[7] == matrixB[7]);
}
END_TEST()

START_TEST(matrix_map)
{
    // (u32 rows, u32 columns, f32 *m, MatrixMap *map, void *user = 0)
}
END_TEST()

START_TEST(matrix_map_at)
{
    // (u32 rows, u32 columns, f32 *m, MatrixMapAt *map, void *user = 0)
}
END_TEST()

internal void
test_matrix(void)
{
    fprintf(stdout, "Testing matrix.h\n");
    test_matrix_add_scalar();
    test_matrix_multiply_scalar();
    test_matrix_add_matrix();
    test_matrix_sub_matrix();
    test_matrix_hadamard_matrix();
    test_matrix_multiply_matrix();
    test_matrix_copy();
    test_matrix_transpose();
    test_matrix_map();
    test_matrix_map_at();
    fprintf(stdout, "Test matrix.h successful!\n\n");
}

#endif