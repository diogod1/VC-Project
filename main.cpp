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
		//std::cout << "Pressione qualquer tecla para continuar...\n";
		//std::cin.get();
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
	cv::namedWindow("VC - VIDEO", cv::WND_PROP_AUTOSIZE);
	cv::namedWindow("VC - MASK", cv::WND_PROP_AUTOSIZE);

	/* Inicia o timer */
	vc_timer();

    /*
	// Cria uma nova imagem IVC
	*/
	IVC *image = vc_image_new(video.width, video.height, 3, 255);
	IVC *image_2 = vc_image_new(video.width, video.height, 3, 255);
	IVC *image_3 = vc_image_new(video.width, video.height, 1, 255);
	IVC *image_4 = vc_image_new(video.width, video.height, 1, 255);
	IVC *image_6 = vc_image_new(video.width, video.height, 1, 255);

	ImageColors *img_colors = (ImageColors *)malloc(sizeof(ImageColors));

	vc_initialize_colors(video.width,video.height,img_colors,1,255);

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

        /*Segmentar o corpo*/
		vc_hsv_resistances_segmentation(image_2,image_3,img_colors);

        vc_binary_close(image_3,image_3,5,5);

		int nlabel;
		OVC *blobs = vc_binary_blob_labelling(image_3, image_6, &nlabel);
		if(blobs != NULL)
			vc_binary_blob_info(image_6, blobs, nlabel);
		
		/*
		// lógica para eliminar blobs pequenos (falhas) - nº de blobs errado no display atm
        for (int i = 0; i < nlabel; i++){
			if(blobs[i].area < 2000) {
				for (int y = blobs[i].y; y < blobs[i].y + blobs[i].height; y++)
			 		for (int x = blobs[i].x; x < blobs[i].x + blobs[i].width; x++)
						image_3->data[y * image_3->width + x] = 0;
				continue;
			}
		}
		nlabel = 0;
		*/
		/*
		OVC *blobs_v2 = vc_binary_blob_labelling(image_3, image_6, &nlabel);
		if(blobs_v2 != NULL)
			vc_binary_blob_info(image_6, blobs, nlabel);
		*/
		

		ResistenceColorList ResColors;
		for (int i = 0; i < nlabel; i++){
			// filtrar apenas pelas resistências para o calculo dos ohms
		 	if(blobs[i].area < 5000 || blobs[i].area > 10000){
				continue;
			}
			if(video.nframe == 1) vc_write_image("../../1.pgm", img_colors->azul);

			ResColors = {0};
			ResColors.lista_verde = 0;
			ResColors.lista_azul = 0;
			ResColors.lista_vermelho = 0;
			ResColors.lista_amarelo = 0;
			ResColors.lista_laranja = 0;
			ResColors.lista_preto = 0;
			ResColors.lista_castanho = 0;
			for (int y = blobs[i].y; y < blobs[i].y + blobs[i].height; y++) {
				for (int x = blobs[i].x; x < blobs[i].x + blobs[i].width; x++) {
					int pos = y * image_3->width + x;
					if(img_colors->verde->data[pos] = 255) ResColors.lista_verde++;
					if(img_colors->azul->data[pos] = 255)  ResColors.lista_azul++;
					if(img_colors->vermelho->data[pos] = 255) ResColors.lista_vermelho++;
					if(img_colors->amarelo->data[pos] = 255) ResColors.lista_amarelo++;
					if (img_colors->laranja->data[pos] == 255) ResColors.lista_laranja++;
					if (img_colors->preto->data[pos] == 255) ResColors.lista_preto++;
					if (img_colors->castanho->data[pos] == 255) ResColors.lista_castanho++;

					//image_3->data[y * image_3->width + x] = 0;
				}
			}

			CorContagemImagem cores[] = {
				{ResColors.lista_preto, img_colors->preto},
				{ResColors.lista_castanho, img_colors->castanho},
				{ResColors.lista_vermelho, img_colors->vermelho},
				{ResColors.lista_laranja, img_colors->laranja},
				{ResColors.lista_amarelo, img_colors->amarelo},
				{ResColors.lista_verde, img_colors->verde},
				{ResColors.lista_azul, img_colors->azul}
			};

			qsort(cores, sizeof(cores) / sizeof(cores[0]), sizeof(CorContagemImagem), compare_cor);
			
			if(video.nframe == 1) calcularResistenciaTotal(cores);

/* 			int top[4] = {0};
    		int *p = (int *)&ResColors;
			int size = sizeof(ResistenceColorList) / sizeof(int);

			for (int i = 0; i < size; i++) {
				for (int j = 0; j < 4; j++) {
					if (p[i] > top[j]) {
						for (int k = 3; k > j; k--) {
							top[k] = top[k - 1];
						}
						top[j] = p[i];
						break;
					}
				}
			} */
			
			//blobs[i].potencia= vc_check_resistence_color(blobs[i].x, blobs[i].y ,blobs[i].width, blobs[i].height, img_colors, ResColors);

			/*
		 	//Corta imagem pelo tamanho do blob
		 	IVC *image_blob = vc_image_new(blobs[i].width, blobs[i].height, 1, 255);
			memcpy(image_blob,image_3,blobs[i].width * blobs[i].height * 3);

			//Verfica cores
		 	for (int y = blobs[i].y; y < blobs[i].y + blobs[i].height; y++) {
				for (int x = blobs[i].x; x < blobs[i].x - blobs[i].width; x++) {
		 			long int pos = y * image_3->bytesperline + x * image_3->channels;		
		 		}
         	}
		    
			free(image_blob);*/
			//vc_free_images(img_colors);
		}
		
		/* ver imagem preto e branco */
		cv::Mat imageToShow = cv::Mat(image_3->height, image_3->width, CV_8UC3);
			for (int y = 0; y < image_3->height; y++) {
				for (int x = 0; x < image_3->width; x++) {
					uchar value = image_3->data[y * image_3->width + x];
					imageToShow.at<cv::Vec3b>(y, x) = cv::Vec3b(value, value, value); // Replicar valor para os três canais
				}
			}
		memcpy(frame.data, imageToShow.data, video.width * video.height * 3);

		for (int i = 0; i < nlabel; i++){
			if(blobs[i].area < 2000)
				continue;
			str = std::string("VALOR: ").append(std::to_string(blobs[i].potencia));
			cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
			cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
		}
		
		str = std::string("BLOB'S DETETADOS: ").append(std::to_string(nlabel));
		cv::putText(frame, str, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
		cv::putText(frame, str, cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 1);

		str = std::string("FRAME: ").append(std::to_string(video.nframe));
		cv::putText(frame, str, cv::Point(20, 170), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
		cv::putText(frame, str, cv::Point(20, 170), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 1);

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);
		memcpy(frame.data, image->data, video.width * video.height * 3);
		cv::imshow("VC - MASK", frame);

		/* Sai da aplica��o, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}

    vc_image_free(image);
	vc_image_free(image_2);
	vc_image_free(image_3);
    
	if (img_colors != NULL) {
        vc_free_images(img_colors);
        free(img_colors);
    }
	// +++++++++++++++++++++++++

	/* Para o timer e exibe o tempo decorrido */
	vc_timer();

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");
	cv::destroyWindow("VC - MASK");

	/* Fecha o ficheiro de v�deo */
	capture.release();

	return 0;
}