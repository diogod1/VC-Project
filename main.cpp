#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2/objdetect/objdetect.hpp> 
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "vc.h"
}

struct ColorRange {
    int minHue;
    int maxHue;
    int minSaturation;
    int maxSaturation;
    int minBrightness;
    int maxBrightness;
};

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
	char videofile[100] = "../../video_resistors.mp4";
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

	/* Leitura de v�deo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video.avi dever� estar localizado no mesmo direct�rio que o ficheiro de c�digo fonte.
	*/
	capture.open(videofile);

	/* Em alternativa, abrir captura de v�deo pela Webcam #0 */
	//capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

	/* Verifica se foi possível abrir o ficheiro de vídeo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
		return 1;
	}

	/* N�mero total de frames no v�deo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do v�deo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolu��o do v�deo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o v�deo */
	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

	/* Inicia o timer */
	vc_timer();

	/*
	// Array com as cores todas
	*/

	ColorRange colors[11] = {
        {25, 45, 35, 64, 45, 90},     // Corpo resisitencia
        {25, 45, 35, 64, 45, 90},     // Vermelho
        {130, 205, 5, 55, 30, 45},    // Azul
        {75, 165, 30, 100, 30, 100},  // Verde
        // {45, 75, 60, 100, 50, 100},   // Amarelo
        // {75, 165, 30, 100, 30, 100},  // Verde
        // {165, 255, 15, 100, 35, 100}, // Azul
        // {220, 320, 30, 100, 30, 100}, // Roxo
        // {0, 360, 0, 10, 20, 80},      // Cinza
        // {0, 360, 0, 0, 100, 100}      // Branco
    };

    /*
	// Cria uma nova imagem IVC
	*/
	IVC *image = vc_image_new(video.width, video.height, 3, 255);
	IVC *image_2 = vc_image_new(video.width, video.height, 3, 255);
	IVC *image_3 = vc_image_new(video.width, video.height, 1, 255);
	IVC *image_vermelho = vc_image_new(video.width, video.height, 1, 255);
	IVC *image_azul = vc_image_new(video.width, video.height, 1, 255);
	IVC *image_6 = vc_image_new(video.width, video.height, 1, 255);
	IVC *image_verde = vc_image_new(video.width, video.height, 1, 255);

	cv::Mat frame;
	while (key != 'q') {
		/* Leitura de uma frame do v�deo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		/* N�mero da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(image->data, frame.data, video.width * video.height * 3);
		// Executa uma fun��o da nossa biblioteca vc
        vc_bgr_to_rgb(image,image_2);
		vc_write_image("../../resis_rgb.pgm", image_2);
        /*Segmentar o corpo*/
        //c_hsv_segmentation(image_2,image_3,30,45,45,80,60,90);
		vc_hsv_segmentation(image_2,image_3,colors[0].minHue,colors[0].maxHue,colors[0].minSaturation,colors[0].maxSaturation,45,90);
        /*Segmentar as cores das linhas -- vermelho*/
        vc_hsv_segmentation(image_2,image_vermelho,340,20,55,70,55,100);
		/*Segmentar as cores das linhas -- azul*/
		vc_hsv_segmentation(image_2,image_azul,130,205,5,55,30,45);
		/*Segmentar as cores das linhas -- verde*/
		vc_hsv_segmentation(image_2,image_verde,75,165,30,100,30,100);
        /*Junta cores com corpo, para formar um blob*/
        for (int y = 0; y < image_3->height; y++) {
            for (int x = 0; x < image_3->width; x++) {
                long int pos = y * image_3->bytesperline + x * image_3->channels;
                if (image_vermelho->data[pos] == 255 || image_azul->data[pos] == 255 || image_verde->data[pos] )
                    image_3->data[pos] = 255;
            }
        }
        vc_binary_close(image_3,image_3,5,5);

		int nlabel;
		OVC *blobs = vc_binary_blob_labelling(image_3, image_6, &nlabel);
		if(blobs != NULL)
			vc_binary_blob_info(image_6, blobs, nlabel);

		/*Identifica cada blob*/
		// for (int i = 0; i < nlabel; i++){
		// 	/*Restringe pela area*/
		// 	if(blobs[i].area < 2000)
		// 		continue;

		// 	//Corta imagem pelo tamanho do blob
		// 	IVC *image_temp = vc_image_new(blobs[i].width, blobs[i].height, 1, 255);
		// 	/*Verfica cores*/
		// 	for (int y = blobs[i].y; y < blobs[i].y + blobs[i].height; y++) {
		// 		for (int x = blobs[i].x; x < blobs[i].x - blobs[i].width; x++) {
		// 			long int pos = y * image_3->bytesperline + x * image_3->channels;


			
		// 		}
        // 	}
		//     free(image_temp);
		// }
		
		str = std::string("BLOB'S DETETADOS: ").append(std::to_string(nlabel));
		cv::putText(frame, str, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
		cv::putText(frame, str, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 1);

        /* ver imagem preto e branco */
        cv::Mat imageToShow = cv::Mat(image_3->height, image_3->width, CV_8UC3);
            for (int y = 0; y < image_3->height; y++) {
                for (int x = 0; x < image_3->width; x++) {
                    uchar value = image_3->data[y * image_3->width + x];
                    imageToShow.at<cv::Vec3b>(y, x) = cv::Vec3b(value, value, value); // Replicar valor para os três canais
                }
            }
        memcpy(frame.data, imageToShow.data, video.width * video.height * 3);

		str = std::string("BLOB'S DETETADOS: ").append(std::to_string(nlabel));
		cv::putText(frame, str, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
		cv::putText(frame, str, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 1);
		

		for (int i = 0; i < nlabel; i++){
			if(blobs[i].area < 2000)
				continue;
			str = std::string("Area: ").append(std::to_string(blobs[i].area));
			cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
			cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
		}

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);

		/* Sai da aplica��o, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}

    vc_image_free(image);
	vc_image_free(image_2);
	vc_image_free(image_3);
    vc_image_free(image_vermelho);
	vc_image_free(image_azul);
	vc_image_free(image_verde);
	// +++++++++++++++++++++++++

	/* Para o timer e exibe o tempo decorrido */
	vc_timer();

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");

	/* Fecha o ficheiro de v�deo */
	capture.release();

	return 0;
}