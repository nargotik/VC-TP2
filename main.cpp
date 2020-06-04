#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "plate-recognizer.h"
}

using namespace cv;
using namespace std;

void vc_timer(void) {
    static bool running = false;
    static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

    if (!running) {
        running = true;
    }
    else {
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

        // Tempo em segundos.
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
        double nseconds = time_span.count();

        std::cout << "Tempo decorrido: " << nseconds << "segundos" << std::endl;
        std::cout << "Pressione qualquer tecla para continuar...\n";
        std::cin.get();
    }
}



int main(void) {
    // V�deo

    char videofile[90] = "C:\\IPCA\\VC_TP2\\VC_TP2\\video2.mp4";
	// 1 to video 1 , 2 to video 2
	int algo = 2; 

	char caracteres[7] = "      ";
    cv::VideoCapture capture;
    struct
    {
        int width, height;
        int ntotalframes;
        int fps;
        int nframe;
    } video;
    // Outros
    std::string str;
    int key = 0;
    int found = 0;


    capture.open(videofile);

    if (!capture.isOpened())
    {
        std::cerr << "Erro ao abrir o ficheiro de v�deo!\n";
        return 1;
    }

    /* Numero total de frames no video */
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    /* Frame rate do video */
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    /* Resolução do video */
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    /* Cria uma janela para exibir o video */
    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

    /* Inicia o timer */
    vc_timer();

	//learn();
	std::string matricula;

    cv::Mat frame;
    while (key != 'q') {
        /* Leitura de uma frame do video */
        capture.read(frame);


        /* Verifica se conseguiu ler a frame */
        if (frame.empty()) break;

        /* Numero da frame a processar */
        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);


        cv::Mat frame2;

        // Cria uma nova imagem IVC
        IVC *image = vc_image_new(video.width, video.height, 3, 255);
		IVC* image_orig = vc_image_new(video.width, video.height, 3, 255);

		//teste(frame);

		cv::GaussianBlur(frame, frame2, Size(17, 17), 0, 0, BORDER_CONSTANT);

        memcpy(image->data, frame2.data, video.width * video.height * 3);
		memcpy(image_orig->data, frame.data, video.width * video.height * 3);
		
        rgb2bgrinvert(image);
		rgb2bgrinvert(image_orig);
		
		OVC blob_matricula[1];
		OVC blobs_caracteres[6];
		
		
		if (processFrame(image, image_orig, blobs_caracteres, blob_matricula, algo)) {
			found++;
			desenha_bounding_box(image_orig, blob_matricula, 1);
			desenha_bounding_box(image_orig, blobs_caracteres, 6);


			matricula = 
				std::string() + 
				blobs_caracteres[0].data + blobs_caracteres[1].data + '-' +
				blobs_caracteres[2].data + blobs_caracteres[3].data + '-' +
				blobs_caracteres[4].data + blobs_caracteres[5].data +
				std::string();
		}
		rgb2bgrinvert(image_orig);
        rgb2bgrinvert(image);

        // Copia dados de imagem da estrutura IVC para uma estrutura cv::Mat
        memcpy(frame.data, image_orig->data, video.width * video.height * 3);

		vc_image_free(image);
		vc_image_free(image_orig);

        // +++++++++++++++++++++++++

        str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
        cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
        cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
        cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

        str = std::string("").append(matricula);
        cv::putText(frame, str, cv::Point(20, 150), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 150), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 0, 255), 3);

		std::string char1 = std::string() + blobs_caracteres[0].data + std::string();
		str = std::string("").append(char1);
		cv::putText(frame, str, cv::Point(blobs_caracteres[0].x, blobs_caracteres[0].y + blobs_caracteres[1].height),cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(255, 0, 0), 2);
		
		std::string char2 = std::string() + blobs_caracteres[1].data + std::string();
		str = std::string("").append(char2);
		cv::putText(frame, str, cv::Point(blobs_caracteres[1].x, blobs_caracteres[1].y + blobs_caracteres[1].height), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(255, 0, 0), 2);
		
		std::string char3 = std::string() + blobs_caracteres[2].data + std::string();
		str = std::string("").append(char3);
		cv::putText(frame, str, cv::Point(blobs_caracteres[2].x, blobs_caracteres[2].y + blobs_caracteres[2].height), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(255, 0, 0), 2);
		
		std::string char4 = std::string() + blobs_caracteres[3].data + std::string();
		str = std::string("").append(char4);
		cv::putText(frame, str, cv::Point(blobs_caracteres[3].x , blobs_caracteres[3].y + blobs_caracteres[3].height), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(255, 0, 0), 2);
		
		std::string char5 = std::string() + blobs_caracteres[4].data + std::string();
		str = std::string("").append(char5);
		cv::putText(frame, str, cv::Point(blobs_caracteres[4].x, blobs_caracteres[4].y + blobs_caracteres[4].height), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(255, 0, 0), 2);
		
		std::string char6 = std::string() + blobs_caracteres[5].data + std::string();
		str = std::string("").append(char6);
		cv::putText(frame, str, cv::Point(blobs_caracteres[5].x, blobs_caracteres[5].y + blobs_caracteres[5].height), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(255, 0, 0), 2);

        /* Exibe a frame */
        cv::imshow("VC - VIDEO", frame);

        /* Sai da aplica��o, se o utilizador premir a tecla 'q' */
        key = cv::waitKey(1);
    }

    /* Para o timer e exibe o tempo decorrido */
    vc_timer();

    /* Fecha a janela */
    cv::destroyWindow("VC - VIDEO");

    /* Fecha o ficheiro de v�deo */
    capture.release();

    return 0;
}