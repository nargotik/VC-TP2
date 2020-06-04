/**
 * Este ficheiro contem as assinaturas de funções que foram criadas a fim de reconhecimento das matriculas propostas no TP2
 * @brief Ficheiro que contem as assinaturas de funções criadas para o processamento de uma imagem a fim de extrair a matricula e seus caracteres
 * @file plate-recognizer.h
 * @date 04 Maio de 2020
 * @author Rafael Pereira <a13871@alunos.ipca.pt>
 * @author Óscar Silva <a14383@alunos.ipca.pt>
 * @author Daniel Torres <a17442@alunos.ipca.pt>
 */

#ifndef VC_TP1_13871_14383_17442_IMAGE_RECOGNIZER_H
#define VC_TP1_13871_14383_17442_IMAGE_RECOGNIZER_H

#include "vc.h"

typedef struct {
	int numeroBlobs;
	OVC blob_matricula;	
	OVC blobs_caracteres[6];
	char caracteres;
	int label;
} VC_ARGS;



int vc_brigten(IVC *src, int value);
void debugSave(char *filen,int id, IVC *src);
int directory_exists(const char *path);
int file_exists(const char *path);
int rgb_to_gray(int r, int g, int b);
void invertImageBinary(IVC *src);
void fillImage(IVC *src, unsigned char value);
float extractBlob(IVC *src, IVC *dst, OVC blob);
float extractBlobBinary(IVC *src, IVC *dst, OVC blob);
int calcula_desvio(int r, int g, int b);
int vc_color_remove(IVC *image, int threshold, int color);

void replaceFrame(IVC* src, IVC* frame);
int vc_medbright(IVC* src);

float extractBlobRGB(IVC* src, IVC* dst, OVC* blob);
int processFrame(IVC* frame, IVC* frame_orig, OVC blobs_caracteres[6], OVC blob_matricula[1], int algo);
int desenha_bounding_box(IVC* src, OVC* blobs, int numeroBlobs);
void getChannel(IVC* src, IVC* out, int channel);
int learnChar(IVC* image_test);
void learn();
int calcula_desvio5(int a, int b, int c, int d, int e);
char predict(IVC* image_test);
#endif //VC_TP1_13871_14383_17442_IMAGE_RECOGNIZER_H
