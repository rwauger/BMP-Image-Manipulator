/*
 * Ryan Auger
 *
 * Assumptions: 
 *       The file must be 24-bit color, without compression, and without
 *       a color map.
 *
 *       Some bmp files do not set the ImageSize field.  This code
 *       prints a warning but does not consider this an error since the
 *       size of the image can be calculated from other values.
 *
 * Command line argument
 *   name of bit mapped image file (bmp file) to read
 *
 * Bugs:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* WARNING: the header is 14 bytes, however on most systems
 * you will find that sizeof(struct Header) is 16 due to alignment
 * Thus trying to read the header with one fread may fail.  So,
 * read each member separately
 */
struct Header
{  
    unsigned short int Type;                 /* Magic identifier            */
    unsigned int Size;                       /* File size in bytes          */
    unsigned short int Reserved1, Reserved2;
    unsigned int Offset;                     /* Offset to image data, bytes */
};

struct InfoHeader
{  
    unsigned int Size;               /* header size in bytes      */
    int Width, Height;               /* Width and height of image */
    unsigned short int Planes;       /* Number of colour planes   */
    unsigned short int Bits;         /* Bits per pixel            */
    unsigned int Compression;        /* Compression type          */
    unsigned int ImageSize;          /* Image size in bytes       */
    int xResolution,yResolution;     /* Pixels per meter          */
    unsigned int Colors;             /* Number of colors         */
    unsigned int ImportantColors;    /* Important colors         */
};

const char Matrix[3][3] = 
{ 
    {  0, -1,  0 },
    { -1,  4, -1 },
    {  0, -1,  0 }
};

#define LINE 256

struct Pixel
{ 
    unsigned char Blue, Green, Red;
}; 


/*----------------------------------------------------------*/

int main(int argc, char *argv[])
{ 
    char filein[LINE];
    char fileout[LINE];
    FILE *fpin;
    FILE *fpout;
    struct InfoHeader infoheader;
    struct Header header;
    int expected_bytes;
    int error_code = 0;
    int row, column;
    int pixel_cols, pixel_rows, pixel_count;
    int items_found;
    struct Pixel one_pixel;

    if (argc != 4)
    {
        printf("Usage: parsebmp filename\n");
        exit(1);
    }
    strcpy(filein, argv[2]);
    strcpy(fileout, argv[3]);
    
    if(strcmp(argv[2], argv[3]) == 0){
      printf("Input and Output file are the same.\n");
      exit(1);
    }
    
    if(strcmp(argv[1],"trunc") != 0 && strcmp(argv[1],"center") != 0 && strcmp(argv[1],"mag") != 0 && strcmp(argv[1],"scale") != 0){
      printf("This command is invalid.\n");
      exit(1);
    }
    
      
    if ((fpin = fopen(filein, "rb")) == NULL)
    { 
        printf("Cannot Open File. %s\n", filein);
        exit (1);
    }
    
    if ((fpout = fopen(fileout, "wb")) == NULL)
    { 
        printf("Cannot Open File. %s\n", filein);
        exit (1);
    }

    /* Read header */
    fread(&header.Type, sizeof(short int), 1, fpin);
    fread(&header.Size, sizeof(int), 1, fpin);
    fread(&header.Reserved1, sizeof(short int), 1, fpin);
    fread(&header.Reserved2, sizeof(short int), 1, fpin);
    fread(&header.Offset, sizeof(int), 1, fpin);

    printf("header.Type = %x\n", header.Type);
    printf("header.Size = %d\n", header.Size);
    printf("header.Offset = %d\n", header.Offset);

    if (header.Type != 0x4D42)
    {
        printf("This does not appear to be a bmp file: %s\n", filein);
        exit(1);
    }
    fread(&infoheader.Size, sizeof(int), 1, fpin);
    fread(&infoheader.Width, sizeof(int), 1, fpin);
    fread(&infoheader.Height, sizeof(int), 1, fpin);
    fread(&infoheader.Planes, sizeof(short int), 1, fpin);
    fread(&infoheader.Bits, sizeof(short int), 1, fpin);
    fread(&infoheader.Compression, sizeof(int), 1, fpin);
    fread(&infoheader.ImageSize, sizeof(int), 1, fpin);
    fread(&infoheader.xResolution, sizeof(int), 1, fpin);
    fread(&infoheader.yResolution, sizeof(int), 1, fpin);
    fread(&infoheader.Colors, sizeof(int), 1, fpin);
    fread(&infoheader.ImportantColors, sizeof(int), 1, fpin);

    printf("infoheader.Size = %d\n", infoheader.Size);
    printf("infoheader.Width = %d\n", infoheader.Width);
    printf("infoheader.Height = %d\n", infoheader.Height);
    printf("infoheader.Planes = %d\n", infoheader.Planes);
    printf("infoheader.Bits = %d\n", infoheader.Bits);
    printf("infoheader.Compression = %d\n", infoheader.Compression);
    printf("infoheader.ImageSize = %d\n", infoheader.ImageSize);
    printf("infoheader.xResolution = %d\n", infoheader.xResolution);
    printf("infoheader.yResolution = %d\n", infoheader.yResolution);
    printf("infoheader.Colors = %d\n", infoheader.Colors);
    printf("infoheader.ImportantColors = %d\n", infoheader.ImportantColors);

    if (header.Offset != 54)
    {
        printf("problem with offset.  Cannot handle color table\n");
	error_code +=1;
    }
    if (infoheader.Size != 40)
    {
        printf("Size is not 40, perhaps a bmp format not handled\n");
	error_code +=2;
    }
    if (infoheader.Planes != 1 || infoheader.Compression != 0)
    {
        printf("Planes or Compression format not handled\n");
	error_code +=4;
    }
    if (infoheader.Bits != 24)
    {
        printf("Only 24 bit color handled\n");
	error_code +=8;
    }
    expected_bytes = (infoheader.Width * infoheader.Height * infoheader.Bits)/8;
    if (expected_bytes != infoheader.ImageSize)
    {
        printf("Problem with image size.  Sometimes this field is not set so we will ignore the error.\n");
	error_code +=16;
    }
    if (expected_bytes + 14 + 40 != header.Size)
    {
        printf("Problem with size in header\n");
	error_code +=32;
    }
    if (infoheader.Colors != 0 || infoheader.ImportantColors != 0)
    {
        printf("Cannot handle color map\n");
	error_code +=64;
    }
    if (error_code != 0 && error_code != 16)
    {
	printf("exit with code %x\n", error_code);
	exit(EXIT_FAILURE);
    }

    printf("Reading pixels\n");

    pixel_rows = infoheader.Height;
    pixel_cols = infoheader.Width;
    pixel_count = 0;
    struct Pixel **Pix;
    struct Pixel **pixel_transform;
    int i, red_result, blue_result, green_result;
    unsigned char new_pixel_color;
    int col, j;
   
    
    //allocation for any of the commands
    Pix = (struct Pixel**)malloc(pixel_rows*sizeof(struct Pixel*));
    
    for(i = 0; i < pixel_rows; ++i){
      Pix[i] = (struct Pixel*)malloc(pixel_cols*sizeof(struct Pixel));
    }
    
    pixel_transform = (struct Pixel**)malloc(pixel_rows*sizeof(struct Pixel*));
    
    for(i = 0; i < pixel_rows; ++i){
      pixel_transform[i] = (struct Pixel*)malloc(pixel_cols*sizeof(struct Pixel));
    }
    
     for (row = 0; row < pixel_rows; row++)
    { 
        for (column = 0; column < pixel_cols; column++)
        { 
	    items_found = fread(&one_pixel, 3, 1, fpin);
            if (items_found != 1)
            {
                printf("failed to read pixel %d at [%d][%d]\n", 
                        pixel_count, row, column);
                exit(1);
            }
            Pix[row][column].Red = one_pixel.Red;
            Pix[row][column].Green = one_pixel.Green;
            Pix[row][column].Blue = one_pixel.Blue;
            
            pixel_count++;
        }
    }
    fclose(fpin);
    
   //Pixel Transform
   for(row = 0; row < pixel_rows; ++row){
      for(col = 0; col < pixel_cols; ++col){
          if (col == 0 || row == 0 || col == (pixel_cols - 1) || row == (pixel_rows - 1) )
            pixel_transform[row][col] = Pix[row][col];
          else{
            red_result = 0;
            green_result = 0;
            blue_result = 0;
            for(i = 0; i < 3; ++i){
               for(j = 0; j < 3; ++j)
               {
                  red_result += Pix[row-1+i][col-1+j].Red * Matrix[i][j];
                  green_result += Pix[row-1+i][col-1+j].Green * Matrix[i][j];
                  blue_result += Pix[row-1+i][col-1+j].Blue * Matrix[i][j];
               }
            }
            if(strcmp(argv[1], "trunc") == 0){
               new_pixel_color = red_result;
               pixel_transform[row][col].Red = new_pixel_color;
               
               new_pixel_color = green_result;
               pixel_transform[row][col].Green = new_pixel_color;
               
               new_pixel_color = blue_result;
               pixel_transform[row][col].Blue = new_pixel_color;
            }
            else if(strcmp(argv[1], "center") == 0){
               red_result += 128;
               if(red_result < 0) red_result = 0;
               if(red_result > 255) red_result = 255;
               new_pixel_color = red_result;
               pixel_transform[row][col].Red = new_pixel_color;
               
               green_result += 128;
               if(green_result < 0) green_result = 0;
               if(green_result > 255) green_result = 255;
               new_pixel_color = green_result;
               pixel_transform[row][col].Green = new_pixel_color;
               
               blue_result += 128;
               if(blue_result < 0) blue_result = 0;
               if(blue_result > 255) blue_result = 255;
               new_pixel_color = blue_result;
               pixel_transform[row][col].Blue = new_pixel_color;
            }
			else if(strcmp(argv[1], "mag") == 0){
               red_result = abs(red_result);
			   if (red_result > 255) red_result = 255;
			   new_pixel_color = red_result;
               pixel_transform[row][col].Red = new_pixel_color;
               
               green_result = abs(green_result);
			   if (green_result > 255) green_result = 255;
			   new_pixel_color = green_result;
               pixel_transform[row][col].Green = new_pixel_color;
               
               blue_result = abs(blue_result);
			   if (blue_result > 255) blue_result = 255;
			   new_pixel_color = blue_result;
               pixel_transform[row][col].Blue = new_pixel_color;
            }
			else if(strcmp(argv[1], "scale") == 0){
               red_result /= 8;
               red_result += 128;
               new_pixel_color = red_result;
               pixel_transform[row][col].Red = new_pixel_color;
			   
               green_result /= 8;
               green_result += 128;
               new_pixel_color = green_result;
               pixel_transform[row][col].Green = new_pixel_color;
               
               blue_result /= 8;
               blue_result += 128;
               new_pixel_color = blue_result;
               pixel_transform[row][col].Blue = new_pixel_color;
            }
               
          }
      }
   }
    
    fwrite(&header.Type, sizeof(short int), 1, fpout);
    fwrite(&header.Size, sizeof(int), 1, fpout);
    fwrite(&header.Reserved1, sizeof(short int), 1, fpout);
    fwrite(&header.Reserved2, sizeof(short int), 1, fpout);
    fwrite(&header.Offset, sizeof(int), 1, fpout);
    
    fwrite(&infoheader.Size, sizeof(int), 1, fpout);
    fwrite(&infoheader.Width, sizeof(int), 1, fpout);
    fwrite(&infoheader.Height, sizeof(int), 1, fpout);
    fwrite(&infoheader.Planes, sizeof(short int), 1, fpout);
    fwrite(&infoheader.Bits, sizeof(short int), 1, fpout);
    fwrite(&infoheader.Compression, sizeof(int), 1, fpout);
    fwrite(&infoheader.ImageSize, sizeof(int), 1, fpout);
    fwrite(&infoheader.xResolution, sizeof(int), 1, fpout);
    fwrite(&infoheader.yResolution, sizeof(int), 1, fpout);
    fwrite(&infoheader.Colors, sizeof(int), 1, fpout);
    fwrite(&infoheader.ImportantColors, sizeof(int), 1, fpout);
    
    for (row = 0; row < pixel_rows; row++)
    { 
        for (column = 0; column < pixel_cols; column++)
        { 
	    items_found = fwrite(&pixel_transform[row][column], 3, 1, fpout);
            if (items_found != 1)
            {
                printf("failed to read pixel %d at [%d][%d]\n", 
                        pixel_count, row, column);
                exit(1);
            }
        }
    }
    fclose(fpout);
    
    for(i = 0; i < pixel_rows; ++i){
      free(Pix[i]);
    }
    free(Pix);
    for(i = 0; i < pixel_rows; ++i){
      free(pixel_transform[i]);
    }
    free(pixel_transform);
       
    return 0;
}
