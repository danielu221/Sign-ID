#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include <iostream>
using namespace cv;
using namespace std;

/*********************************************************************ZMIENNE GLOBALNE**********************************/

// Zadeklarowanie liczby podpisow wystepujacych na liscie
const int numberOfSignatures = 5;


// Lista podpisow autentycznych
Mat List = imread("podpisy.jpg"); Mat List_frame; Mat List_original;

// Tablica przechowuj¹ca wyciête podpisy z listy
Mat Signature[numberOfSignatures];

// Tablica przechowuj¹ca podpisy w skali szaroœci
Mat Signature_gray[numberOfSignatures];

// Tablica przechowuj¹ca binarne wersje podpisów
Mat Signature_bin[numberOfSignatures];

// Matryca przechowujaca badany podpis
Mat Signature_tested = imread("podpisBadany.jpg");
Mat Signature_tested_gray; Mat Signature_tested_bin; Mat Signature_tested2; Mat Signature_tested_snippet;
Mat Signature_tested_frame; Mat Signature_tested_original;

// liczba okreœlaj¹ca najmniejsz¹ rozdzielczoœæ z listy
int minResolution = List.cols;
////////////////////////////////////////////////ZMIENNE ZWI¥ZANE Z WYKRYWANIEM I WYCINANIEM SignatureÓW /////////////////////////////////

// Flagi informuj¹ce nas, czy klikniêto w okna podisu badanego i okna podisu z listu
int signatureTestedClick = 0; int ListClick = 0;

// Aktualny rozmiar struktularnego elementu, jego wartoœæ zmieniamy za pomoc¹ trackbarów
int rozmiar = 9; int sizeListElement = 9;

//Przechowuje wektory okreœlaj¹ce wierzcho³ki prostok¹tów do wyciêcia
vector<Rect> croppedSignature; vector<Rect> croppedSignatureTested;

////////////////////////////////////////////////ZMIENNE DLA PORÓWNANIA OBRAZÓW POPRZEZ NA£O¯ENIE/////////////////////////////////

// Tablica przechowujaca bitowe AND
Mat And_signatures[numberOfSignatures - 1];

// Matryca przechowujaca wynik porownywania
Mat Signature_final_laying; Mat Signature_final_laying_bin;

////////////////////////////////////////////////ZMIENNE DLA PORÓWNANIA METOD¥ MATCHING METHOD/////////////////////////////////

//Okresla rozmiary fragmentu do porownywania
int snippetX = 30; int snippetY = 30;

//Przechowuje matrycê ze znormalizowanymi wynikami porównania metod¹ matchingMethod
Mat result;

//Okreœla metodê porównania matchingMethod
int match_method = 1;

//Zmienne przechowuj¹ce wspó³rzêdne zwi¹zane z mysz¹
int x_move, y_move, x_snippet_edge, y_snippet_edge, flag = 0;

//Zmienna, która mówi nam ile podpisów z listy ma badany fragment w podobnym miejscu do podpisu badanego
int similarToTested = 0;



/************************************************************FUNKCJE*******************************************************************/

/***************************************
Funkcja detectLetters- wykrywa obszar z podpisami
****************************************/
vector<Rect> detectLetters(Mat img, int rozmiar)
{
	vector<Rect> boundRect;
	Mat img_gray, img_sobel, img_threshold, element;
	cvtColor(img, img_gray, CV_BGR2GRAY);
	Sobel(img_gray, img_sobel, CV_8U, 1, 0, 3, 1, 0, BORDER_DEFAULT);
	threshold(img_sobel, img_threshold, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY);
	element = getStructuringElement(MORPH_RECT, Size(rozmiar, 3));
	morphologyEx(img_threshold, img_threshold, CV_MOP_CLOSE, element);
	vector< vector< Point> > contours;
	findContours(img_threshold, contours, 0, 1);
	vector<vector<Point> > contours_poly(contours.size());
	for (int i = 0; i < contours.size(); i++)
		if (contours[i].size()>100)
		{
			approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
			Rect appRect(boundingRect(Mat(contours_poly[i])));
			if (appRect.width>appRect.height)
				boundRect.push_back(appRect);
		}
	return boundRect;
}
/***************************************
Funkcja findMinResolution-  znajduje z danej tablicy obrazów obraz o najmniejszej rozdzielczosci i zapisuje go pod nazwa "Signature_minResolution.jpg"
****************************************/
void findMinResolution(Mat Images_array[])
{
	for (int i = 0; i < numberOfSignatures; i++)
	{
		if (Images_array[i].cols < minResolution)
		{
			minResolution = Images_array[i].cols;
		}
	}
	for (int i = 0; i < numberOfSignatures; i++)
	{
		if (Images_array[i].cols == minResolution)
		{
			imwrite("Signature_minResolution.jpg", Images_array[i]);
		}
	}
}

/***************************************
Funkcja equalResolution- skaluje wszystkie obrazy z tablicy Signature[], aby mia³y rozdzielczoœæ, jak obraz o nazwie podanej w argumencie
****************************************/
void equalResolution(string image_name)
{
	for (int i = 0; i < numberOfSignatures; i++)
	{
		Mat minResolution = imread(image_name);
		Mat ScaledSignature = Signature[i];
		resize(ScaledSignature, ScaledSignature, minResolution.size());
		Signature[i] = ScaledSignature;
	}
	Mat minResolution = imread(image_name);
	Mat src = Signature_tested;
	resize(src, src, minResolution.size());
	Signature_tested = src;
}

/***************************************
Funkcja onMouseMatching-obs³uga myszy dla matchingMethod, umo¿liwia wybranie fragmentu podpisu badanego do porównywania z innymi podpisami
****************************************/
static void onMouseMatching(int event, int x, int y, int, void*)
{
	x_move = x;
	y_move = y;

	if (event == EVENT_LBUTTONDOWN)
	{
		if (flag == 0)
		{
			x_snippet_edge = x;
			y_snippet_edge = y;
			flag = 1;
		}

	}
}

/***************************************
Funkcja onMouseCropTested-przy kliknieciu wycina podpis badany w wybranej przez nas ramce
****************************************/
static void onMouseCropTested(int event, int x, int y, int, void*)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		signatureTestedClick = 1;
	}
}

/***************************************
Funkcja onMouseCropList-przy kliknieciu wycina podpisy z listy w wybranej przez nas ramce
****************************************/
static void onMouseCropList(int event, int x, int y, int, void*)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		ListClick = 1;
	}
}

/***************************************
Funkcja toGrayscale- przejscie na skalê szarosci dla wszystkich podpisów z tablicy Signature[]
****************************************/
void toGrayscale()
{
	for (int i = 0; i < numberOfSignatures; i++)
	{
		cvtColor(Signature[i], Signature_gray[i], CV_BGR2GRAY);

	}
	cvtColor(Signature_tested, Signature_tested_gray, CV_BGR2GRAY);
}
/***************************************
Funkcja toBin- dokonuje binaryzacji dla wszystkich podpisów z tablicy Signature_gray[]
****************************************/
void toBin()
{
	for (int i = 0; i < numberOfSignatures; i++)
	{
		threshold(Signature_gray[i], Signature_bin[i], 125, 255, 0);
	}
	threshold(Signature_tested_gray, Signature_tested_bin, 125, 255, 0);
}
/***************************************
Funkcja overlap- znajduje czesc wspolna obrazu utworzonego ze wszystkich podpisów w bazie i podpisu badanego
****************************************/
void overlap()
{
	// Na³o¿enie na siebie wszystkich podpisów
	bitwise_and(Signature_bin[0], Signature_bin[1], And_signatures[0]);
	for (int i = 0; i < numberOfSignatures - 2; i++)
	{
		bitwise_and(And_signatures[i], Signature_bin[i + 2], And_signatures[i + 1]);

	}

	// Na³o¿enie na siebie sumy wszystkich podpisów i badanego podpisu z wagami 0.5, zapisanie wyniku do Signature_final_laying_bin
	addWeighted(And_signatures[numberOfSignatures - 2], 0.5, Signature_tested_bin, 0.5, 0, Signature_final_laying);

	// toBin obrazu uzyskanego poprzez na³o¿enie na siebie sumy wszystkich podpisów i badanego podpisu z wagami 0.5
	threshold(Signature_final_laying, Signature_final_laying_bin, 125, 255, 0);
}

/***************************************
Funkcja matchingMethod- znajduje obszar, w ktorym wyciety fragment podpisu badanego jest porownywany do podpisów z list
****************************************/
void matchingMethod(int, void*)
{

	for (int i = 0; i < numberOfSignatures; i++)
	{
		/// Obraz wyœwietalny
		Mat img_display;
		Signature[i].copyTo(img_display);

		/// Stworzenie macierzy wynikowej
		int result_cols = Signature[i].cols - Signature_tested_snippet.cols + 1;
		int result_rows = Signature[i].rows - Signature_tested_snippet.rows + 1;

		result.create(result_rows, result_cols, CV_32FC1);

		/// Wykonanie operacji porównania i normalizacja
		matchTemplate(Signature[i], Signature_tested_snippet, result, match_method);
		normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

		/// Wybranie obszaru o najwiêkszym podobobieñstwie do fragmentu wzorca
		double minVal; double maxVal; Point minLoc; Point maxLoc;
		Point matchLoc;

		/// Znajduje globalne maximum i minimum w danej tablicy
		minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

		/// Dla metod porówynawania: SQDIFF i SQDIFF_NORMED, najbardziej podobne obszary maj¹ minimalne warotoœci. Dla pozosta³ych maxima.
		if (match_method == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED)
		{
			matchLoc = minLoc;
		}
		else
		{
			matchLoc = maxLoc;
		}

		/// Obrysowanie obszaru o najwiêkszym podobobieñstwie ramk¹ o wymiarach fragmentu badanego podpisu
		rectangle(img_display, matchLoc, Point(matchLoc.x + Signature_tested_snippet.cols, matchLoc.y + Signature_tested_snippet.rows), Scalar(0,0,255), 2, 8, 0);
		rectangle(result, matchLoc, Point(matchLoc.x + Signature_tested_snippet.cols, matchLoc.y + Signature_tested_snippet.rows), Scalar(0,0,255), 2, 8, 0);
		imwrite("WYNIK_DOPASOWANIE" + to_string(i) + ".jpg", img_display);
		if (((matchLoc.x)<(x_snippet_edge + snippetX / 5)) && ((matchLoc.x)>(x_snippet_edge - snippetX / 5)) && ((matchLoc.y)<(y_snippet_edge + snippetY / 5)) && ((matchLoc.y)>(y_snippet_edge - snippetY / 5)))
		{
			similarToTested++;
		}
	}
	Mat displayTested;
	Signature_tested.copyTo(displayTested);
	rectangle(displayTested, Point(x_snippet_edge, y_snippet_edge), Point(x_snippet_edge + snippetX, y_snippet_edge + snippetY), Scalar(0, 0, 0), 2, 8, 0);
	imwrite("WYNIK_DOPASOWANIE_BADANY.jpg", displayTested);
	cout << "2.Wyniki dopasowania fragmentu do pozostalych podpisow zostaly zapisane pod nazwa \"WYNIK_DOPASOWANIEXX.jpg\" do folderu z programem. Mozesz je przeanalizowac" << endl << endl;
	return;
}

/***************************************
Funkcja blackPixels- znajduje i zwraca liczbê czarnych pikseli na danym obrazie
****************************************/
int blackPixels(Mat E)
{
	int blackPix = 0;
	for (int i = 0; i < E.cols; i++)
	{
		for (int j = 0; j < E.rows; j++)
		{
			uchar*p = E.data + E.cols*j + i;
			if (*p == 0){
				blackPix++;
			}
		}
	}
	return blackPix;
}

/***************************************
Funkcja compabilityPercent- oblicza procent zgodnoœci badanego obrazu z obrazami z listy ( stosunek czarnych pikseli dla czesci wspolnej badanego podpisu i matrycy do czarnych pikseli badanego podpisu)
****************************************/
int compabilityPercent(int badany, int uzyskany)
{
	float percent;

	float tested_float = badany;
	float final_float = uzyskany;
	percent = (final_float / tested_float) * 100;
	int percentint = percent;
	return percent;
}

/***************************************
Funkcja trackbarTested- obs³ugiwana w trakcie zmiany trackabaru, zaznacza obszar podpisu badanego do wyciêcia
****************************************/
void trackbarTested(int, void*)
{
	if (rozmiar < 9)
	{
		rozmiar = 9;
	}
	Signature_tested_frame = Signature_tested_original.clone();
	croppedSignatureTested = detectLetters(Signature_tested_frame, rozmiar);
	rectangle(Signature_tested_frame, croppedSignatureTested[0], Scalar(0, 0, 255), 1, 8, 0);
	imshow("Badany", Signature_tested_frame);
}

/***************************************
Funkcja trackbarList- obs³ugiwana w trakcie zmiany trackbaru, zaznacza obszar podpisów z listy do wyciêcia
****************************************/
void trackbarList(int, void*)

{
	List_frame = List_original.clone();
	if (sizeListElement < 9)
	{
		sizeListElement = 9;
	}
	croppedSignature = detectLetters(List_frame, sizeListElement);
	for (int i = 0; i < croppedSignature.size(); i++)
	{

		rectangle(List_frame, croppedSignature[i], Scalar(0, 0, 255), 1, 8, 0);
		imshow("List", List_frame);
	}
}

int main()
{

	//Sprawdzamy, czy za³adowano listê podpisów i podpis badany
	if (!List.data || !Signature_tested.data)
	{
		printf("Obrazy nie zaladowaly sie poprawnie!");
		getchar();
		return -1;
	}
	cout << "****WITAJ W PROGRAMIE SignatureID-weryfikator podpisow****" << endl << endl;

	cout << "Rozpoczynamy TEST PIERWSZY, w tym tescie podpis badany jest porownywany z suma podpisow z listy. Wynik dopasowania pokazuje w ilu procentach podpis badany pokrywa sie z nalozonymi na siebie podpisami z listy" << endl << endl;
	cout << "1. Na okienku z wyswietlonym podpisem badanym, wybierz za pomoca suwaka rozmiar ramki (podpis powinien byc objety mozliwie waska ramka), a nastepnie kliknij myszka w dowolnym miejscu obrazu." << endl << endl;


	//Operacje kopiowania
	Signature_tested_original = Signature_tested.clone();
	Signature_tested_frame = Signature_tested.clone();
	List_frame = List.clone();
	List_original = List.clone();

	namedWindow("Badany", CV_WINDOW_AUTOSIZE);

	//Operacja wycinania dla podpisu badanego
	croppedSignatureTested = detectLetters(Signature_tested, rozmiar);
	Signature_tested = Mat(Signature_tested, croppedSignatureTested[0]);
	rectangle(Signature_tested_frame, croppedSignatureTested[0], Scalar(0, 0, 255), 1, 8, 0);
	imshow("Badany", Signature_tested_frame);

	//Trackbar steruj¹cy wielkoœci¹ elemetu strukturalnego w metodzie detectLetters
	createTrackbar("croppedSignatureTested", "Badany", &rozmiar, 40, trackbarTested);
	//W³¹czenie obs³ugi myszy, klikniêcie w okno powoduje wyciêcie obrazu w obszarze czerwonej ramki
	setMouseCallback("Badany", onMouseCropTested, 0);
	while (1)
	{
		if (signatureTestedClick == 1)
		{
			cout << "Wycieto zaznaczony podpis badany." << endl << endl;
			Signature_tested = Mat(Signature_tested_original, croppedSignatureTested[0]);
			destroyWindow("Badany");
			break;
		}

		if (waitKey(30) == 27)
		{
			break;
		}

		imshow("Badany", Signature_tested_frame);
	}
	////////////////////////////////////////////////////////////////////////////////////////////////
	namedWindow("List", CV_WINDOW_AUTOSIZE);
	cout << "2. Na okienku z wyswietlona lista podpisow wybierz za pomoca suwaka rozmiar ramek obejmujacych podpisy ( kazdy podpis powinien byc objety mozliwie waska ramka)" << endl;
	cout << "Po wybraniu rozmiaru ramek kliknij myszka w dowolnym miejscu obrazu." << endl << endl;

	//Operacja wycinania dla podpisów z listy
	croppedSignature = detectLetters(List, sizeListElement);
	for (int i = 0; i < croppedSignature.size(); i++)
	{
		rectangle(List_frame, croppedSignature[i], Scalar(0, 0, 255), 1, 8, 0);
		Signature[i] = Mat(List, croppedSignature[i]);
		imshow("List", List_frame);
		

	}
	//Trackbar steruj¹cy wielkoœci¹ elemetu strukturalnego w metodzie detectLetters
	createTrackbar("WycinekList", "List", &sizeListElement, 40, trackbarList);
	//W³¹czenie obs³ugi myszy, klikniêcie w okno powoduje wyciêcie obrazów w obszarze czerwonych ramek
	setMouseCallback("List", onMouseCropList, 0);
	while (1)
	{
		if (ListClick == 1)
		{

			for (int i = 0; i < croppedSignature.size(); i++)
			{

				Signature[i] = Mat(List_original, croppedSignature[i]);
				destroyWindow("List");
				
			}
			cout << "Wycieto zaznaczone podpisy." << endl << endl;
			break;
		}

		if (waitKey(30) == 27)
		{
			break;
		}
		imshow("List", List_frame);
	}



	findMinResolution(Signature);

	equalResolution("Signature_minResolution.jpg");

	toGrayscale();

	toBin();

	overlap();

	cout << "3.Koniec testu pierwszego, wynik poznasz po kolejnym tescie." << endl << endl;
	cout << "Rozpoczynamy TEST DRUGI, w tym tescie wybrany przez Ciebie fragment podpisu badanego jest porownywany z kazdym podpisem z listy. Wynik dopasowania jest dodatkowo zapisywany do folderu, gdzie znajduje sie program SingatureID. " << endl << endl;
	cout << "1.Zaznacz wybrany przez Ciebie fragment do porownywania z innymi podpisami." << endl << endl;

	namedWindow("PodpisBadany", CV_WINDOW_AUTOSIZE);
	setMouseCallback("PodpisBadany", onMouseMatching, 0);
	while (1)
	{
		Signature_tested2 = Signature_tested.clone();
		if (waitKey(20) == 27){ break; }
		if (flag == 0)
		{
			if (x_move + snippetX > Signature_tested.cols)
			{
				x_move = Signature_tested.cols - snippetX;
			}
			if (y_move + snippetY > Signature_tested.rows)
			{
				y_move = Signature_tested.rows - snippetY;
			}
			rectangle(Signature_tested2, Point(x_move, y_move), Point(x_move + snippetX, y_move + snippetY), Scalar(0, 0, 255), 2, 8, 0);
			imshow("PodpisBadany", Signature_tested2);

		}
		if (flag == 1)
		{
			if (x_snippet_edge + snippetX > Signature_tested.cols)
			{
				x_snippet_edge = Signature_tested.cols - snippetX;
			}
			if (y_snippet_edge + snippetY > Signature_tested.rows)
			{
				y_snippet_edge = Signature_tested.rows - snippetY;
			}
			Signature_tested_snippet = Mat(Signature_tested, Rect(Point(x_snippet_edge, y_snippet_edge), Point(x_snippet_edge + snippetX, y_snippet_edge + snippetY)));
			break;
		}




	}

	matchingMethod(0, 0);

	/// Prezentacja wyników uzyskanych w testach
	Mat wyniki = Mat(720, 1024, CV_8UC3);
	rectangle(wyniki, Point(0, 0), Point(1024, 720), Scalar(0, 0, 0), -1, 8, 0);
	putText(wyniki, "Zgodnosc wedlug testu nr1 (nalozenie): " + to_string(compabilityPercent(blackPixels(Signature_tested_bin), blackPixels(Signature_final_laying_bin))) + "%", Point(10, 200), CV_FONT_NORMAL, 1, Scalar(255, 255, 255), 1, 5, 0);
	putText(wyniki, "Zgodnosc wedlug testu nr2 (wycinek): ", Point(10, 250), CV_FONT_NORMAL, 1, Scalar(255, 255, 255), 1, 5, 0);
	putText(wyniki, "Fragment wyciety zgadza sie z " + to_string(similarToTested) + " na " + to_string(numberOfSignatures) + " podpisow w bazie", Point(10, 300), CV_FONT_NORMAL, 1, Scalar(255, 255, 255), 1, 5, 0);
	if ((compabilityPercent(blackPixels(Signature_tested_bin), blackPixels(Signature_final_laying_bin))) > 50 && similarToTested >= 1)
	{
		putText(wyniki, "Podpis JEST PRAWDZIWY!", Point(200, 400), CV_FONT_NORMAL, 1, Scalar(0, 255, 0), 2, 5, 0);

	}
	else
	{
		putText(wyniki, "Podpis JEST FALSZYWY!", Point(200, 400), CV_FONT_NORMAL, 1, Scalar(0, 0, 255), 2, 5, 0);
	}
	namedWindow("Wynik", CV_WINDOW_AUTOSIZE);
	imshow("Wynik", wyniki);
	namedWindow("Uzyskany", CV_WINDOW_NORMAL);
	imshow("Uzyskany", Signature_final_laying_bin);
	namedWindow("Uzyskany2", CV_WINDOW_NORMAL);
	imshow("Uzyskany2", And_signatures[3]);

	cout << endl << "Mozesz wyjsc z programu klikajac ESC." << endl;
	waitKey(0);
}