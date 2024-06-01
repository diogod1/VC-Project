//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define VC_DEBUG
#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM VÍDEO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int width, height;
	int ntotalframes;
	int fps;
	int nframe;
} Video;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	unsigned char *data;
	int width, height;
	int channels;			// Binário/Cinzentos=1; RGB=3
	int levels;				// Binário=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM BLOB (OBJECTO)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// Área
	int xc, yc;					// Centro-de-massa
	int perimeter;				// Per�metro
	int label;					// Etiqueta
} OVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DA CONFIGURAÇÃO DE CORES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
    int minHue;
    int maxHue;
    int minSaturation;
    int maxSaturation;
    int minValue;
    int maxValue;
} ColorRange;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DAS CORES PRESENTES NO FRAME
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
    IVC *corpo;
    IVC *preto;
    IVC *castanho;
    IVC *vermelho;
    IVC *laranja;
    IVC *amarelo;
    IVC *verde;
    IVC *azul;
    IVC *roxo;
    IVC *cinza;
    IVC *branco;
} ImageColors;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DOS CONTADORES DAS CORES NO FRAME
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
    int lista_preto;
    int lista_castanho;
    int lista_vermelho;
    int lista_laranja;
    int lista_amarelo;
    int lista_verde;
    int lista_azul;
    int lista_roxo;
    int lista_cinza;
    int lista_branco;
} ResistenceColorList;

typedef struct {
    int multiplicador;
    char digito;
    int contagem;
    IVC *imagem;
    int xmin;
} CorContagemImagem;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DOS CONTADORES DAS CORES NO FRAME
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct{
    int valor;
    int count;
}ContadorResistencia;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROT�TIPOS DE FUN��ES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
IVC *vc_image_new(int width, int height, int channels, int levels);
IVC *vc_image_free(IVC *image);

// FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC *vc_read_image(char *filename);
int vc_write_image(char *filename, IVC *image);
int vc_gray_negative(IVC *srcdst);
int vc_rgb_negative(IVC *srcdst);
int vc_rgb_get_red_gray(IVC *srcdst);
int vc_rgb_get_green_gray(IVC *srcdst);
int vc_rgb_get_blue_gray(IVC *srcdst);
int vc_rgb_to_gray(IVC *src, IVC *dst);
int vc_rgb_to_hsv(IVC *src, IVC *dst);
int vc_hsv_segmentation(IVC *src, IVC *dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);
int vc_hsv_resistances_segmentation(IVC *src, IVC *dst, ImageColors *img_colors);
int vc_scale_gray_to_color_palette(IVC *src, IVC *dst);
int vc_gray_to_binary(IVC *src, IVC *dst, int threshold);
int vc_gray_to_binary_global_mean(IVC *src, IVC *dst);
int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel);
int vc_gray_to_binary_niblac(IVC *src, IVC *dst, int kernel, float k);
int vc_binary_dilate(IVC *src, IVC *dst, int kernel);
int vc_binary_erode(IVC *src, IVC *dst, int kernel);
int vc_binary_open(IVC *src, IVC *dst, int kernelErode, int kernelDilate);
int vc_binary_close(IVC *src, IVC *dst, int kernelErode, int kernelDilate);
OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels);
int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs, int areaRelevant, bool bSortAreaDesc);
int vc_blob_to_gray_scale(IVC *src, IVC *dst, int nlabels);
int vc_draw_center_of_gravity(IVC *img, OVC *blob, int comp);
int vc_draw_bounding_box(IVC *img, OVC *blob);
int vc_gray_histogram_show(IVC *src, IVC *dst);
int vc_gray_histogram_equalization(IVC *src, IVC *dst);
int vc_gray_edge_prewitt(IVC *src, IVC *dst, float th);
int vc_gray_edge_sobel(IVC *src, IVC *dst, float th);
int vc_gray_lowpass_mean_filter(IVC *src, IVC *dst, int kernelsize);
int compare(const void *a, const void *b);
int vc_gray_lowpass_median_filter(IVC *src, IVC *dst, int kernelsize);

// Novas funções
int vc_bgr_to_rgb(IVC *src, IVC *dst);
void vc_initialize_colors(int width, int height, ImageColors *img_colors, int channels, int levels);
void vc_free_images(ImageColors *img_colors);
ResistenceColorList vc_check_resistence_color(int xpos, int ypos, int width, int height, ImageColors *img_colors, int videoWidth);
bool vc_check_resistence_body(int xpos, int ypos, int width, int height, IVC *image);
void swap_cores(CorContagemImagem* cor1, CorContagemImagem* cor2);
void swap_blobs(OVC** blob1, OVC** blob2);
OVC *vc_binary_blob_labelling_custom(IVC *src, IVC *dst, int *nlabels, int xpos, int ypos, int blobWidth, int blobHeight);
int vc_binary_blob_info_custom(IVC *src, OVC *blobs, int nblobs, int areaRelevant, int xpos, int ypos, int blobWidth, int blobHeight);
