// #include <mpi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
#include "tpool.h"

//定義平滑運算的次數
#define NSmooth 1000

/*********************************************************/
/*變數宣告：                                             */
/*  bmpHeader    ： BMP檔的標頭                          */
/*  bmpInfo      ： BMP檔的資訊                          */
/*  **BMPSaveData： 儲存要被寫入的像素資料               */
/*  **BMPData    ： 暫時儲存要被寫入的像素資料           */
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;                                               
RGBTRIPLE **BMPData = NULL;                                                   

/*********************************************************/
/*函數宣告：                                             */
/*  readBMP    ： 讀取圖檔，並把像素資料儲存在BMPSaveData*/
/*  saveBMP    ： 寫入圖檔，並把像素資料BMPSaveData寫入  */
/*  swap       ： 交換二個指標                           */
/*  **alloc_memory： 動態分配一個Y * X矩陣               */
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
/*變數宣告：                                             */
/*  *infileName  ： 讀取檔名                             */
/*  *outfileName ： 寫入檔名                             */
/*  startwtime   ： 記錄開始時間                         */
/*  endwtime     ： 記錄結束時間                         */
/*********************************************************/
	char *infileName = "input.bmp";
    char *outfileName = "output.bmp";
	double startwtime = 0.0, endwtime=0;

	//MPI_Init(&argc,&argv);
	
	//記錄開始時間
	//startwtime = MPI_Wtime();

	//讀取檔案
    if ( readBMP( infileName) )
        printf("Read file successfully!!\n");
    else 
        printf("Read file fails!!\n");

	//動態分配記憶體給暫存空間
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
	
	//進行多次的平滑運算
	for(int count = 0; count < NSmooth ; count ++) {
		//把像素資料與暫存指標做交換
		swap(BMPSaveData, BMPData);
		//進行平滑運算
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
 	//寫入檔案
    if ( saveBMP( outfileName ) ) {
        printf("Save file successfully!!\n");
    } else {
        printf("Save file fails!!\n");
    }
	//得到結束時間，並印出執行時間
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
/* 讀取圖檔                                              */
/*********************************************************/
int readBMP(char *fileName)
{
	//建立輸入檔案物件	
        // ifstream bmpFile( fileName, ios::in | ios::binary );
		FILE *bmpFile = fopen(fileName, "rb");
 
        //檔案無法開啟
        if ( !bmpFile ){
			printf("It can't open file!!\n");
            return 0;
        }
 
        //讀取BMP圖檔的標頭資料
    	// bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );
		fread( ( char* ) &bmpHeader, sizeof( BMPHEADER ), 1, bmpFile );
 
        //判決是否為BMP圖檔
        if( bmpHeader.bfType != 0x4d42 ){
            printf("This file is not .BMP!!\n");
            fclose(bmpFile);
            return 0;
        }
 
        //讀取BMP的資訊
        // bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );
		fread( ( char* ) &bmpInfo, sizeof( BMPINFO ), 1, bmpFile );
        
        //判斷位元深度是否為24 bits
        if ( bmpInfo.biBitCount != 24 ){
            printf("The file is not 24 bits!!\n");
            return 0;
        }

        //修正圖片的寬度為4的倍數
        while( bmpInfo.biWidth % 4 != 0 )
        	bmpInfo.biWidth++;

        //動態分配記憶體
        BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
        
        //讀取像素資料
    	//for(int i = 0; i < bmpInfo.biHeight; i++)
        //	bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
	    // bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);
		fread( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight, 1, bmpFile);
		
        //關閉檔案
        // bmpFile.close();
        fclose(bmpFile);
        return 1;
 
}
/*********************************************************/
/* 儲存圖檔                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
 	//判決是否為BMP圖檔
        if( bmpHeader.bfType != 0x4d42 ){
            printf("This file is not .BMP!!\n");
            return 0;
        }
        
 	//建立輸出檔案物件
        // ofstream newFile( fileName,  ios:: out | ios::binary );
        FILE *newFile = fopen(fileName, "wb");
        //檔案無法建立
        if ( !newFile ){
            printf("The File can't create!!\n");
            return 0;
        }
 	
        //寫入BMP圖檔的標頭資料
        // newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );
        fwrite( ( char* )&bmpHeader, 1, sizeof( BMPHEADER ), newFile );
	//寫入BMP的資訊
        // newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

        fwrite( ( char* )&bmpInfo, 1, sizeof( BMPINFO ), newFile );
        //寫入像素資料
        //for( int i = 0; i < bmpInfo.biHeight; i++ )
        //        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
        // newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

        fwrite( ( char* )BMPSaveData[0], 1, bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight, newFile );
        //寫入檔案
        // newFile.close();
        fclose(newFile);
        return 1;
 
}


/*********************************************************/
/* 分配記憶體：回傳為Y*X的矩陣                           */
/*********************************************************/
RGBTRIPLE **alloc_memory(int Y, int X )
{        
	//建立長度為Y的指標陣列
        RGBTRIPLE **temp = malloc(Y * sizeof(RGBTRIPLE *));
	    RGBTRIPLE *temp2 = malloc(X * Y * sizeof(RGBTRIPLE));
        memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
        memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//對每個指標陣列裡的指標宣告一個長度為X的陣列 
        for( int i = 0; i < Y; i++){
                temp[ i ] = &temp2[i*X];
        }
 
        return temp;
 
}
/*********************************************************/
/* 交換二個指標                                          */
/*********************************************************/
void swap(RGBTRIPLE **a, RGBTRIPLE **b)
{
	RGBTRIPLE **temp;
	temp = a;
	a = b;
	b = temp;
}

