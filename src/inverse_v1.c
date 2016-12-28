#ifndef INVERSE_V1_C
#define INVERSE_V1_C

//#define defLocalMain


typedef struct Tipo_s_vector  {
	float row[4];
} s_vector;

typedef struct Tipo_s_matrix  {
    s_vector cols[4];
} s_matrix;


 float cofactor_ij_v1( s_matrix xdata * mat, unsigned long int col, unsigned int row){
    xdata float cofactorF;

    static  const unsigned long int sel0[] = { 1,0,0,0 };
    static  const unsigned long int sel1[] = { 2,2,1,1 };
    static  const unsigned long int sel2[] = { 3,3,3,2 };
    
    // Let's first define the 3x3 matrix:
     xdata unsigned long int col0 = sel0[col];
     xdata unsigned long int col1 = sel1[col];
     xdata unsigned long int col2 = sel2[col];
     xdata unsigned long int row0 = sel0[row];
     xdata unsigned long int row1 = sel1[row];
     xdata unsigned long int row2 = sel2[row];
    
    // Compute the float sign-mask:
     xdata unsigned	long int signpos  = (col + row) & 1;
    
    // Computer the det of the 3x3 matrix:
    // 
    //   [ a b c ]
    //   [ d e f ] = aei - ahf + dhc - dbi + gbf - gec = (aei + dhc + gbf) - (ahf + dbi + gec)
    //   [ g h i ] 
    //
    
     xdata float		pos_part1 = mat->cols[col0].row[row0] * mat->cols[col1].row[row1] * mat->cols[col2].row[row2]; // aei
     xdata float		pos_part2 = mat->cols[col0].row[row1] * mat->cols[col1].row[row2] * mat->cols[col2].row[row0]; // dhc
     xdata float		pos_part3 = mat->cols[col0].row[row2] * mat->cols[col1].row[row0] * mat->cols[col2].row[row1]; // gbf
                                                                                                        
     xdata float		neg_part1 = mat->cols[col0].row[row0] * mat->cols[col1].row[row2] * mat->cols[col2].row[row1]; // ahf
     xdata float		neg_part2 = mat->cols[col0].row[row1] * mat->cols[col1].row[row0] * mat->cols[col2].row[row2]; // dbi
     xdata float		neg_part3 = mat->cols[col0].row[row2] * mat->cols[col1].row[row1] * mat->cols[col2].row[row0]; // gec	
    
     xdata float		pos_part  = pos_part1 + pos_part2 + pos_part3;
     xdata float		neg_part  = neg_part1 + neg_part2 + neg_part3;
     xdata float		det3x3	  = pos_part - neg_part;
    
    
     cofactorF  = det3x3;
     if (signpos)
     	cofactorF*=-1;
    
     return cofactorF;
}


void cofactor_v1(s_matrix xdata * output, s_matrix xdata * source){
    xdata long int col;
    xdata long int row;
    
    for ( col = 0 ; col < 4 ; col++ ){
        for ( row = 0; row < 4; row++ ){
            output->cols[col].row[row] =  cofactor_ij_v1(source, col, row);
        }
    }
}

float determinant_v1( s_matrix xdata *   source,  s_matrix xdata *  cofactor){
    xdata long int col;
    xdata float det = 0.0f;
    for ( col = 0; col < 4; col++ )
        det += source->cols[col].row[0] * cofactor->cols[col].row[0];
    return det;
}

void transpose_v1(s_matrix xdata *   output,  s_matrix xdata *   source){
    xdata long int index1 = 0;
    xdata long int index2 = 0;
    
    for ( index1 = 0; index1 < 4; index1++ )
    {
        for ( index2 = 0; index2 < 4; index2++ )
        {
            output->cols[index2].row[index1] = source->cols[index1].row[index2];
        }
    }
}

void mul_v1(s_matrix xdata *   output,  s_matrix xdata *   source,  float factor){
    xdata long int col;
    xdata long int row;
    
    for ( col = 0 ; col < 4 ; col++ ){
        for ( row = 0; row < 4; row++ ){
            output->cols[col].row[row] =  source->cols[col].row[row] * factor;
        }
    }
}

void inverse_v1(s_matrix xdata *   output,  s_matrix xdata *   source){
    xdata s_matrix cof;
    xdata s_matrix adj;
    xdata float    oodet;
    
    // InvM = (1/det(M)) * Transpose(Cofactor(M))
    
    cofactor_v1(&cof, source);
    oodet = 1.0f / determinant_v1(source, &cof);
    transpose_v1(&adj, &cof);
    mul_v1(output, &adj, oodet);
}

 void vMultiplyMatrix4x4_Vector(s_matrix xdata *matrix, s_vector xdata *vectorIn, s_vector xdata *vectorOut){
	 xdata unsigned char row,col;
	 xdata float fAccum;
	 for (row=0;row<4;row++){
		 fAccum=0.0;
		 for (col=0;col<4;col++){
			 fAccum+=matrix->cols[col].row[row]*vectorIn->row[col];
		 }
		vectorOut->row[row]=fAccum;
	 }
 }

 static xdata s_matrix matrix_4x4_inv;
 void vSolve4x4System(float xdata *matrix_coeffs, float xdata *vector_data,float xdata *vector_result){
 	inverse_v1(&matrix_4x4_inv,(s_matrix xdata *)matrix_coeffs);
	vMultiplyMatrix4x4_Vector(&matrix_4x4_inv, (s_vector xdata *)vector_data, (s_vector xdata *)vector_result);
 }
 
#ifdef defLocalMain

 static xdata s_matrix matrix_4x4_test={
1.0299,
0.7394,
0.708,
0.5,
1.1718,
1.176,
0.828,
0.832,
0.879,
0.629,
0.855,
0.602,
1,
1,
1,
1
};

void vTestInvMatrix(void){
 	inverse_v1(&matrix_4x4_inv,&matrix_4x4_test);
}


s_vector vector_4x4={1,2,3,4};
s_vector vector_result;

 s_matrix matrix_4x4={
	 1,0,0,0,
	 0,1,0,0,
	 0,0,1,0,
	 0,0,0,1
 };
 s_matrix matrix_4x4_inv;

 s_matrix matrix_4x4_2={
	 1,2,3,4,
	 0,1,0,0,
	 0,0,1,0,
	 0,0,0,1
 };
 s_matrix matrix_4x4_inv2;


 void main(void){
	 vSolve4x4System(&matrix_4x4_2, &vector_4x4,&vector_result);

	  inverse_v1(&matrix_4x4_inv,&matrix_4x4);
	  inverse_v1(&matrix_4x4_inv,&matrix_4x4_2);
	  inverse_v1(&matrix_4x4_inv2,&matrix_4x4_inv);

 }
#endif
#endif // INVERSE_V1_C

