// #include <mpi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
#include "tpool.h"

//�w�q���ƹB�⪺����
#define NSmooth 1000

/*********************************************************/
/*�ܼƫŧi�G                                             */
/*  bmpHeader    �G BMP�ɪ����Y                          */
/*  bmpInfo      �G BMP�ɪ���T                          */
/*  **BMPSaveData�G �x�s�n�Q�g�J���������               */
/*  **BMPData    �G �Ȯ��x�s�n�Q�g�J���������           */
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;                                               
RGBTRIPLE **BMPData = NULL;                                                   

/*********************************************************/
/*��ƫŧi�G                                             */
/*  readBMP    �G Ū�����ɡA�ç⹳������x�s�bBMPSaveData*/
/*  saveBMP    �G �g�J���ɡA�ç⹳�����BMPSaveData�g�J  */
/*  swap       �G �洫�G�ӫ���                           */
/*  **alloc_memory�G �ʺA���t�@��Y * X�x�}               */
/*********************************************************/
int readBMP( char *fileName);        //read file
int saveBMP( char *fileName);        //save file
void swap(RGBTRIPLE **a, RGBTRIPLE **b);
RGBTRIPLE **alloc_memory( int Y, int X );        //allocate memory

typedef struct __param {
    int i, j;
} param_t;


void *smooth(void *param) 
{
	param_t args = *(param_t *) param;
	int i = args.i;
	int j = args.j;
	int Top = i>0 ? i-1 : bmpInfo.biHeight-1;
	int Down = i<bmpInfo.biHeight-1 ? i+1 : 0;
	int Left = j>0 ? j-1 : bmpInfo.biWidth-1;
	int Right = j<bmpInfo.biWidth-1 ? j+1 : 0;
	
	RGBTRIPLE *TMP = malloc(sizeof(RGBTRIPLE));
	TMP->rgbBlue = (double) (BMPData[i][j].rgbBlue+BMPData[Top][j].rgbBlue+BMPData[Top][Left].rgbBlue+BMPData[Top][Right].rgbBlue+BMPData[Down][j].rgbBlue+BMPData[Down][Left].rgbBlue+BMPData[Down][Right].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/9+0.5;
	TMP->rgbGreen = (double) (BMPData[i][j].rgbGreen+BMPData[Top][j].rgbGreen+BMPData[Top][Left].rgbGreen+BMPData[Top][Right].rgbGreen+BMPData[Down][j].rgbGreen+BMPData[Down][Left].rgbGreen+BMPData[Down][Right].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/9+0.5;
	TMP->rgbRed = (double) (BMPData[i][j].rgbRed+BMPData[Top][j].rgbRed+BMPData[Top][Left].rgbRed+BMPData[Top][Right].rgbRed+BMPData[Down][j].rgbRed+BMPData[Down][Left].rgbRed+BMPData[Down][Right].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/9+0.5;
	return (void *)TMP;
}

int main(int argc,char *argv[])
{
/*********************************************************/
/*�ܼƫŧi�G                                             */
/*  *infileName  �G Ū���ɦW                             */
/*  *outfileName �G �g�J�ɦW                             */
/*  startwtime   �G �O���}�l�ɶ�                         */
/*  endwtime     �G �O�������ɶ�                         */
/*********************************************************/
	char *infileName = "input.bmp";
    char *outfileName = "output.bmp";
	double startwtime = 0.0, endwtime=0;

	//MPI_Init(&argc,&argv);
	
	//�O���}�l�ɶ�
	//startwtime = MPI_Wtime();

	//Ū���ɮ�
    if ( readBMP( infileName) )
        printf("Read file successfully!!\n");
    else 
        printf("Read file fails!!\n");

	//�ʺA���t�O���鵹�Ȧs�Ŷ�
    BMPData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
			
	param_t *args = malloc(sizeof(param_t));
	/*
	param_t **args = malloc(bmpInfo.biHeight * sizeof(param_t *));
	for(int i = 0; i<bmpInfo.biHeight ; i++){
		args[i] = malloc(bmpInfo.biWidth * sizeof(param_t));
	}

	for(int i = 0; i<bmpInfo.biHeight ; i++)
		for(int j = 0; j<bmpInfo.biWidth ; j++){
			args[i][j]->i = i;
			args[i][j]->j = j;
		}
	}
	*/
	
	tpool_t pool = tpool_create(4);
	tpool_future_t futures[bmpInfo.biHeight][bmpInfo.biWidth]; // pointer type
	
	//�i��h�������ƹB��
	for(int count = 0; count < NSmooth ; count ++) {
		//�⹳����ƻP�Ȧs���а��洫
		swap(BMPSaveData, BMPData);
		//�i�業�ƹB��
		for(int i = 0; i<bmpInfo.biHeight ; i++) {
			for(int j = 0; j<bmpInfo.biWidth ; j++) {
				args->i = i;
				args->j = j;
				futures[i][j] = tpool_apply(pool, smooth, (void *) args);
				// futures[i][j] = tpool_apply(pool, smooth, (void *) &args[i][j]);
			}
		}
		
		for(int i = 0; i<bmpInfo.biHeight ; i++) {
			for(int j = 0; j<bmpInfo.biWidth ; j++) {
				RGBTRIPLE *result = tpool_future_get(futures[i][j], 0 /* blocking wait */);
				BMPSaveData[i][j] = *result;
				tpool_future_destroy(futures[i][j]);
				free(result);
			}
		}
	}
	
	tpool_join(pool);
 	//�g�J�ɮ�
    if ( saveBMP( outfileName ) ) {
        printf("Save file successfully!!\n");
    } else {
        printf("Save file fails!!\n");
    }
	//�o�쵲���ɶ��A�æL�X����ɶ�
        //endwtime = MPI_Wtime();
    	//cout << "The execution time = "<< endwtime-startwtime <<endl ;

	free(BMPData[0]);
 	free(BMPSaveData[0]);
 	free(BMPData);
 	free(BMPSaveData);
 	//MPI_Finalize();

    return 0;
}

/*********************************************************/
/* Ū������                                              */
/*********************************************************/
int readBMP(char *fileName)
{
	//�إ߿�J�ɮת���	
        // ifstream bmpFile( fileName, ios::in | ios::binary );
		FILE *bmpFile = fopen(fileName, "rb");
 
        //�ɮ׵L�k�}��
        if ( !bmpFile ){
			printf("It can't open file!!\n");
            return 0;
        }
 
        //Ū��BMP���ɪ����Y���
    	// bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );
		fread( ( char* ) &bmpHeader, sizeof( BMPHEADER ), 1, bmpFile );
 
        //�P�M�O�_��BMP����
        if( bmpHeader.bfType != 0x4d42 ){
            printf("This file is not .BMP!!\n");
            fclose(bmpFile);
            return 0;
        }
 
        //Ū��BMP����T
        // bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );
		fread( ( char* ) &bmpInfo, sizeof( BMPINFO ), 1, bmpFile );
        
        //�P�_�줸�`�׬O�_��24 bits
        if ( bmpInfo.biBitCount != 24 ){
            printf("The file is not 24 bits!!\n");
            return 0;
        }

        //�ץ��Ϥ����e�׬�4������
        while( bmpInfo.biWidth % 4 != 0 )
        	bmpInfo.biWidth++;

        //�ʺA���t�O����
        BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
        
        //Ū���������
    	//for(int i = 0; i < bmpInfo.biHeight; i++)
        //	bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
	    // bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);
		fread( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight, 1, bmpFile);
		
        //�����ɮ�
        // bmpFile.close();
        fclose(bmpFile);
        return 1;
 
}
/*********************************************************/
/* �x�s����                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
 	//�P�M�O�_��BMP����
        if( bmpHeader.bfType != 0x4d42 ){
            printf("This file is not .BMP!!\n");
            return 0;
        }
        
 	//�إ߿�X�ɮת���
        // ofstream newFile( fileName,  ios:: out | ios::binary );
        FILE *newFile = fopen(fileName, "wb");
        //�ɮ׵L�k�إ�
        if ( !newFile ){
            printf("The File can't create!!\n");
            return 0;
        }
 	
        //�g�JBMP���ɪ����Y���
        // newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );
        fwrite( ( char* )&bmpHeader, 1, sizeof( BMPHEADER ), newFile );
	//�g�JBMP����T
        // newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

        fwrite( ( char* )&bmpInfo, 1, sizeof( BMPINFO ), newFile );
        //�g�J�������
        //for( int i = 0; i < bmpInfo.biHeight; i++ )
        //        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
        // newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

        fwrite( ( char* )BMPSaveData[0], 1, bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight, newFile );
        //�g�J�ɮ�
        // newFile.close();
        fclose(newFile);
        return 1;
 
}


/*********************************************************/
/* ���t�O����G�^�Ǭ�Y*X���x�}                           */
/*********************************************************/
RGBTRIPLE **alloc_memory(int Y, int X )
{        
	//�إߪ��׬�Y�����а}�C
        RGBTRIPLE **temp = malloc(Y * sizeof(RGBTRIPLE *));
	    RGBTRIPLE *temp2 = malloc(X * Y * sizeof(RGBTRIPLE));
        memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
        memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//��C�ӫ��а}�C�̪����Ыŧi�@�Ӫ��׬�X���}�C 
        for( int i = 0; i < Y; i++){
                temp[ i ] = &temp2[i*X];
        }
 
        return temp;
 
}
/*********************************************************/
/* �洫�G�ӫ���                                          */
/*********************************************************/
void swap(RGBTRIPLE **a, RGBTRIPLE **b)
{
	RGBTRIPLE **temp;
	temp = a;
	a = b;
	b = temp;
}

