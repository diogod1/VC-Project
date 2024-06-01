#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2/objdetect/objdetect.hpp> 
#include <opencv2\highgui.hpp>
#include <opencv2/imgproc.hpp>
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

		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
		double nseconds = time_span.count();

		std::cout << "Tempo decorrido: " << nseconds << "segundos" << std::endl;
	}
}

int main(void) {
	char videofile[100] = "../../video_resistors.mp4";
	cv::VideoCapture capture;
	
	Video video;
	std::string str;
	int key = 0, pos_y = 0;

	capture.open(videofile);

	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
		return 1;
	}

	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	// Cria uma janela para exibir o vídeo
	cv::namedWindow("VC - VIDEO", cv::WND_PROP_AUTOSIZE);

	// Inicia o timer
	vc_timer();

	// Criação das imagens IVC 
	IVC *image = vc_image_new(video.width, video.height, 3, 255);
	IVC *image_rgb = vc_image_new(video.width, video.height, 3, 255);
	IVC *image_res_segmented = vc_image_new(video.width, video.height, 1, 255);
	IVC *image_res_blobs = vc_image_new(video.width, video.height, 1, 255);
	IVC *imageBlobPrimeiraCor = vc_image_new(video.width, video.height, 1, 255);
	IVC *imageBlobSegundaCor = vc_image_new(video.width, video.height, 1, 255);
	IVC *imageBlobTerceiraCor = vc_image_new(video.width, video.height, 1, 255);

	// Alocação de memória e criação das imagens para as cores presentes no frame
	ImageColors *img_colors = (ImageColors *)malloc(sizeof(ImageColors));
	vc_initialize_colors(video.width,video.height,img_colors,1,255);

	//Inicializar o contador das resistencias
	std::vector<ContadorResistencia> contadorResistencia;

	cv::Mat frame;
	cv::Mat frameFinal;
	while (key != 'q') {
		// Leitura de uma frame do vídeo
		capture.read(frame);

		// Verifica se conseguiu ler a frame
		if (frame.empty()) break;
		
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		// Cópia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(image->data, frame.data, video.width * video.height * 3);

		// Executa uma fun��o da nossa biblioteca vc
        vc_bgr_to_rgb(image,image_rgb);

        // Segmentar o corpo
		vc_hsv_resistances_segmentation(image_rgb,image_res_segmented,img_colors);

		// Limpar Ruido das imagens e dilatar
        vc_binary_open(image_res_segmented,image_res_segmented,1,7);

		// Dilatar as cores
		vc_binary_open(img_colors->azul,img_colors->azul,1,3);
		vc_binary_open(img_colors->castanho,img_colors->castanho,1,3);
		vc_binary_open(img_colors->preto,img_colors->preto,1,3);		

		int nlabel;
		OVC *blobs = vc_binary_blob_labelling(image_res_segmented, image_res_blobs, &nlabel);
		if(blobs != NULL)
			vc_binary_blob_info(image_res_blobs, blobs, nlabel, 0, false);

		ResistenceColorList ResColors;
		for (int i = 0; i < nlabel; i++){
			// filtrar apenas pelas resistências para o calculo dos ohms
		 	if(blobs[i].area < 4000 || blobs[i].area > 10000){
				continue;
			}
			
			// Detetar a partir de uma certa linha
			if(blobs[i].y <  100){
				continue;
			}

			// check para verificar se há resitencias-- correr blob
			bool resitence_check = vc_check_resistence_body(blobs[i].x, blobs[i].y ,blobs[i].width, blobs[i].height, img_colors->corpo);
			if(resitence_check == true){
				ResColors = vc_check_resistence_color(blobs[i].x, blobs[i].y ,blobs[i].width, blobs[i].height, img_colors, video.width);
			}else{
				continue;
			}

			CorContagemImagem cores[] = {
				{1,'0', ResColors.lista_preto, img_colors->preto, INT_MAX},
				{10,'1', ResColors.lista_castanho, img_colors->castanho, INT_MAX},
				{100,'2', ResColors.lista_vermelho, img_colors->vermelho, INT_MAX},
				{1000, '3', ResColors.lista_laranja, img_colors->laranja, INT_MAX},
				//{10000, '4', ResColors.lista_amarelo, img_colors->amarelo, INT_MAX},
				{100000,'5', ResColors.lista_verde, img_colors->verde, INT_MAX},
				{1000000,'6', ResColors.lista_azul, img_colors->azul, INT_MAX}
			};

			CorContagemImagem temp;
			int n = sizeof(cores) / sizeof(cores[0]);
			
			// Ordenar as riscas/cores mais abundantes na resistência 
			for (int x = 0; x < n; ++x) 
			{
				for (int y = x + 1; y < n; ++y) 
				{
					if (cores[x].contagem < cores[y].contagem) 
					{
						temp = cores[x];
						cores[x] = cores[y];
						cores[y] = temp;
					}
				}
			}

			// 2 cores iguais
			if(cores[2].contagem < 200) {
				cores[2] = cores[0];
			}

			// 3 cores iguais
			if(cores[1].contagem < 200) {
				cores[1] = cores[0];
			}

			int nlabelCores;
			OVC *blobsPrimeiraCor = vc_binary_blob_labelling_custom(cores[0].imagem, imageBlobPrimeiraCor, &nlabelCores, blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height);
			if(blobsPrimeiraCor != NULL)
				vc_binary_blob_info_custom(imageBlobPrimeiraCor, blobsPrimeiraCor, nlabelCores, 190, blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height);

			OVC *blobsSegundaCor = vc_binary_blob_labelling_custom(cores[1].imagem, imageBlobSegundaCor, &nlabelCores, blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height);
			if(blobsSegundaCor != NULL)
				vc_binary_blob_info_custom(imageBlobSegundaCor, blobsSegundaCor, nlabelCores, 190, blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height);

			OVC *blobsTerceiraCor = vc_binary_blob_labelling_custom(cores[2].imagem, imageBlobTerceiraCor, &nlabelCores, blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height);
			if(blobsTerceiraCor != NULL)
				vc_binary_blob_info_custom(imageBlobTerceiraCor, blobsTerceiraCor, nlabelCores, 190, blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height);

			// não há blobs para as três riscas da resistência
			if(blobsPrimeiraCor == NULL || blobsSegundaCor == NULL || blobsTerceiraCor == NULL) continue;

			// Ordenar as resistências consoante o centro de massa ( da esquerda para a direita )
			if(blobsPrimeiraCor->xc > blobsSegundaCor->xc) {
				swap_cores(&(cores[0]), &(cores[1]));
				swap_blobs(&blobsPrimeiraCor, &blobsSegundaCor);
			} 
			if(blobsSegundaCor->xc > blobsTerceiraCor->xc) {
				swap_cores(&(cores[1]), &(cores[2]));
				swap_blobs(&blobsSegundaCor, &blobsTerceiraCor);
			} 
			if(blobsPrimeiraCor->xc > blobsSegundaCor->xc) {
				swap_cores(&(cores[0]), &(cores[1]));
				swap_blobs(&blobsPrimeiraCor, &blobsSegundaCor);
			}

			// Cálculo do valor dos Ohms da resistência
			char fullString[3];
			fullString[0] = cores[0].digito;
			fullString[1] = cores[1].digito;
			fullString[2] = '\0';
			int valor = atoi(fullString) * cores[2].multiplicador;

			// Cálculo das resistências para apresentação no último frame
			if(blobs[i].y > 200 && blobs[i].y < 208 ){
				bool resistencia_igual = false;
				for(int i = 0;i < contadorResistencia.size();i++){
					// Caso encontre um registo para a mesma resistência, adicionar 1 ao contador
					if(contadorResistencia[i].valor == valor){
						contadorResistencia[i].count += 1;
						resistencia_igual = true;
					}	
				}
				// Caso seja a primeira resistencia deste valor, comerçar contador 1
				if (resistencia_igual == false){
					contadorResistencia.push_back({valor,1});
				}
			}

			str = std::to_string(valor) + "-Ohms";
			cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
			cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);
			str = "X: " + std::to_string(blobs[i].x);
			cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y + blobs[i].height + 15), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
			cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y + blobs[i].height + 15), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);
			str = "Y: " + std::to_string(blobs[i].y);
			cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y + blobs[i].height + 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
			cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y + blobs[i].height + 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);

			// Bounding Box a usar funções do OPENCV
			cv::rectangle(frame, cv::Point(blobs[i].x, blobs[i].y), cv::Point(blobs[i].x + blobs[i].width, blobs[i].y + blobs[i].height ), cv::Scalar(255, 255, 0), 2);
			cv::circle(frame, cv::Point(blobs[i].xc, blobs[i].yc), 1, cv::Scalar(255, 255, 0), 3);
		}

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

		// Exibe a frame
		cv::imshow("VC - VIDEO", frame);

		// Sai da aplicação, se o utilizador premir a tecla 'q'
		key = cv::waitKey(1);
	}

	// Cria o frame final a branco - 255 255 255
	frameFinal = cv::Mat(video.height,video.width,CV_8UC3, cv::Scalar(255,255,255));
	
	// Apresentação final das resistências
	for(int i=0; i<contadorResistencia.size();i++){
		str = std::to_string(contadorResistencia[i].count) + std::string(" RESISTENCIAS DE ") + std::to_string(contadorResistencia[i].valor) + std::string(" Ohms");
		pos_y += 25;
		cv::putText(frameFinal, str, cv::Point(20, pos_y), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 1);
	}

	// Apresentar frame final com o resultado
	cv::imshow("VC - VIDEO",frameFinal);
	// Primir tecla para sair
	cv::waitKey(0);

	// Liberar toda a memoria allocada
    vc_image_free(image);
	vc_image_free(image_rgb);
	vc_image_free(image_res_segmented);
	vc_image_free(image_res_blobs);
	vc_image_free(imageBlobPrimeiraCor);
	vc_image_free(imageBlobSegundaCor);
	vc_image_free(imageBlobTerceiraCor);

	if (img_colors != NULL) {
        vc_free_images(img_colors);
        free(img_colors);
    }

	// Para o timer e exibe o tempo decorrido
	vc_timer();

	// Fecha a janela
	cv::destroyWindow("VC - VIDEO");

	// Fecha o ficheiro de v�deo
	capture.release();

	return 0;
}