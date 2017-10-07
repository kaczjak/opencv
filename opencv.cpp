#include "stdafx.h"
#include <iostream>
#include <fstream>

#include "opencv2\core\core.hpp"
#include "opencv2\imgproc\imgproc.hpp"
#include "opencv2\highgui\highgui.hpp"
#include <math.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <map>

//TODO: Na podanym obrazie znale�� najlepiej punktowane s�owo, zaznaczy� je czerwonym prostok�tem, poda� jego punktacje na podstawie LUT z warto�cimi punktowymi liter

class letter  //Klasa do przechowywania informacji o literze na planszy
{
public:
	cv::Moments moment;
	cv::Point2f wsp;
	char recChar;

	int premiaLiterowa;
	int premiaWyrazowa;
	int punktyLitera;

	int sumaPunktowLitery;

};

class slowo    //Klasa do przechowywania informacji o s�owie na planszy
{
public:
	std::vector<letter>litery;
	int premiaWyrazowa;
	int punktySuma;
};

cv::Point2f odleglosc(cv::Point rog, cv::Mat obrazek)
{

	cv::Point2f najblizszy = cv::Point2f(obrazek.cols, obrazek.rows);
	int odlmin = obrazek.cols + obrazek.rows;

	for (int y = 0; y < obrazek.rows; y++)
	{
		for (int x = 0; x < obrazek.cols; x++)
		{
			if (obrazek.at<uchar>(y, x) == 255)
			{
				int odl = abs(x - rog.x) + abs(y - rog.y);
				if (odl < odlmin)
				{
					odlmin = odl;
					najblizszy.x = x;
					najblizszy.y = x;
					najblizszy.y = y;
				}
			}
		}
	}

	cv::circle(obrazek, najblizszy, 5, cv::Scalar(255, 0, 0), 2, 8, 0);
	return najblizszy;
}

std::multimap<char, std::vector<double>> loadBase(std::string filename) {
	std::multimap<char, std::vector<double>>baza;
	std::ifstream plikin(filename, std::ios::in);
	std::string tmp, liczba;
	std::vector<double>temp;

	char tempchar;
	int k = 0;
	while (!plikin.eof())
	{
		if (k == 8)
		{
			baza.insert(std::make_pair(tempchar, temp));
			temp.clear();
			k = 0;
		}
		std::getline(plikin, tmp);
		if (k == 0)
		{
			tempchar = char(tmp[0]);
		}
		else
		{
			temp.push_back(std::stod(tmp));
		}
		k++;
	}
	baza.insert(std::make_pair(tempchar, temp));
	return baza;
}

char recogniseLetter(letter& l, std::multimap<char, std::vector<double>>base)
{
	std::map<double, char>match;
	double temp;
	double r0, r1, r2, r3,r4;
	char c;
	for (auto i = base.begin(); i != base.end(); i++)
	{
		r0 = 1*abs((l.moment.m00 - i->second[0]));
		r1 = 0.000001*abs((l.moment.m01 - i->second[1]));
		r2 = 0.000000001*abs((l.moment.m02 - i->second[2]));
		r3 = 0.00000000001*abs((l.moment.m03 - i->second[3]));
		r4 = 0.000001*abs((l.moment.m10 - i->second[4]));
		temp = r0+r2+r3+r4;
		c = i->first;
		match.insert(std::make_pair(temp, c));
	}
	l.recChar = match.begin()->second;
	return match.begin()->second;
}

std::vector<std::vector<cv::Point> > filterRectangle(std::vector<std::vector<cv::Point> >conturs2, cv::Mat canny)
{
	std::vector<std::vector<cv::Point> > contursTemp;
	cv::Rect rectangle;
	for (int i = 0; i < conturs2.size(); i++)
	{
		rectangle = cv::boundingRect(conturs2[i]);
		if (rectangle.height >(canny.cols / 50) && rectangle.height < (canny.cols / 23))
		{
			contursTemp.push_back(conturs2[i]);
		}
	}
	return contursTemp;
}

std::vector<letter>filterDup(std::vector<letter>letters, std::vector<std::vector<cv::Point> >&conturs2)
{
	double tresh = 0.25;
	std::vector<std::vector<cv::Point> >contursOut;
	std::vector<letter>out;

	out.push_back(letters[letters.size() - 1]);
	contursOut.push_back(conturs2[letters.size() - 1]);

	for (int i = 0; i < letters.size() - 1; i++)
	{
		if (abs(letters[i].wsp.x - letters[i + 1].wsp.x)+(letters[i].wsp.y - letters[i + 1].wsp.y)>tresh)
		{
			out.push_back(letters[i]);
			contursOut.push_back(conturs2[i]);
		}
	}
	conturs2 = contursOut;
	return out;
}

std::vector<std::vector<cv::Point> >filterArea(std::vector<std::vector<cv::Point> >conturs, cv::Mat canny)
{
	std::vector<std::vector<cv::Point> >conturs2;
	for (auto c : conturs)
	{
		if (cv::contourArea(c) < (canny.cols * canny.rows / 800) && cv::contourArea(c) >
			(canny.cols * canny.rows / 115000))conturs2.push_back(c);
	}
	return conturs2;
}

void wczytajPremie (int tablica[15][15], std::string filename)
{
	std::ifstream plik(filename, std::ios::in);
	std::string tmp;
	std::string temp;
	for (int i = 0; i < 15; i++)
	{
		std::getline(plik, tmp);
		for (int j = 0; j < 15; j++)
		{
			temp = tmp[j];
			tablica[i][j] = std::stod(std::string(temp));
		}
	}

}

std::map<char, int>wczytajWartosciLiter(std::string fileName)
{
	std::map<char, int>wartosci;

	std::string tmp, punkty;
	std::ifstream  plik(fileName, std::ios::in);
	while (!plik.eof())
	{
		std::getline(plik, tmp);
		if (tmp != "")
		{
			punkty = tmp[1];
			std::pair<char, int>in = std::make_pair(char(tmp[0]), std::stod(punkty));
			wartosci.insert(in);
		}
	}

	return wartosci;
}

void zamienWspolrzedneNaPlansze(std::vector <letter>letters, letter literyNaPlanszy[15][15], cv::Mat obrazek)
{
	double wymiarPoz = obrazek.cols;
	double wymiarPio = obrazek.rows;
	double wymiarPolaPoz = wymiarPoz / 15;
	double wymiarPolaPio = wymiarPio / 15;
	int x, y;
	for (auto i = letters.begin(); i != letters.end(); i++)
	{
		for (int a = 0; a < 15; a++)
		{
			if (i->wsp.x > a*wymiarPolaPoz && i->wsp.x < (a + 1)*wymiarPolaPoz)x = a;
		}
		for (int b = 0; b < 15; b++)
		{
			if (double(i->wsp.y+(wymiarPolaPio/2)) > b*wymiarPolaPio && double(i->wsp.y+(wymiarPolaPio/2)) < (b + 1)*wymiarPolaPio)y=b;
		}
		literyNaPlanszy[y][x] = *i;
	}


}

void wyswietlLiteryNaPlanszy(letter literyNaPlanszy[15][15])
{
	for (int i = 0; i < 15; i++)
	{
		for (int j = 0; j < 15; j++)
		{
			if (literyNaPlanszy[i][j].recChar == ' ')std::cout << "_";
			else std::cout << literyNaPlanszy[i][j].recChar;
		}
		std::cout << std::endl;
	}
}

void uzupelnijPunktacjeLiter(letter literyNaPlanszy[15][15], int premieLiterowe[15][15], int premieWyrazowe[15][15], std::map<char, int> wartosciLiter)
{
	for (int i = 0; i < 15; i++)
	{
		for (int j = 0; j < 15; j++)
		{
			if (literyNaPlanszy[i][j].wsp.x != 0)
			{
				literyNaPlanszy[i][j].premiaLiterowa = premieLiterowe[i][j];
				literyNaPlanszy[i][j].premiaWyrazowa = premieWyrazowe[i][j];
				literyNaPlanszy[i][j].punktyLitera = wartosciLiter.find(literyNaPlanszy[i][j].recChar)->second;
			}
		}
	}
}

std::vector<slowo>znajdzSlowaNaPlanszy(letter literyNaPlanszy[15][15])
{
	std::vector<slowo>slowaNaPlanszy;
		for (int i = 0; i < 15; i++)
		{
			for (int j = 0; j < 15; j++)
			{
				if (literyNaPlanszy[i][j].wsp.x != 0)
				{
					slowo tmp;
					while (literyNaPlanszy[i][j].wsp.x != 0 && j < 15)
					{
						tmp.litery.push_back(literyNaPlanszy[i][j]);
						j++;
					}
					slowaNaPlanszy.push_back(tmp);
				}
			}
		}
	for (int i = 0; i < 15; i++)
	{
		for (int j = 0; j < 15; j++)
		{
			if (literyNaPlanszy[j][i].wsp.x != 0)
			{
				slowo tmp;
				while (literyNaPlanszy[j][i].wsp.x != 0 && j < 15)
				{
					tmp.litery.push_back(literyNaPlanszy[j][i]);
					j++;
				}
				slowaNaPlanszy.push_back(tmp);
			}
		}
	}

	return slowaNaPlanszy;
}

std::vector<slowo>filtrujS�owaNaPlanszy(std::vector<slowo>slowaNaPlanszy)
{
	std::vector<slowo>out;
	for (auto i = slowaNaPlanszy.begin(); i != slowaNaPlanszy.end(); i++)
	{
		if (i->litery.size() > 1)out.push_back(*i);
	}
	return out;
}

void wyswietSlowa(std::vector<slowo>slowaNaPlanszy)
{
	for (auto i = slowaNaPlanszy.begin(); i != slowaNaPlanszy.end(); i++)
	{
		for (auto j = i->litery.begin(); j != i->litery.end(); j++)
		{
			std::cout << j->recChar;
		}
		std::cout << std::endl;
	}
}

void policzPunktySlowNaPlanszy(std::vector<slowo>& slowaNaPlanszy)
{
	for (auto i = slowaNaPlanszy.begin(); i != slowaNaPlanszy.end(); i++)
	{
		i->premiaWyrazowa = 1;
		for (auto j = i->litery.begin(); j != i->litery.end(); j++)
		{
			j->sumaPunktowLitery=(j->punktyLitera) * (j->premiaLiterowa);
			if ((j->premiaWyrazowa) > (i->premiaWyrazowa))i->premiaWyrazowa = j->premiaWyrazowa;
		}
	}

	for (auto i = slowaNaPlanszy.begin(); i != slowaNaPlanszy.end(); i++)
	{
		i->punktySuma = 0;
		for (auto j = i->litery.begin(); j != i->litery.end(); j++)
		{
			i->punktySuma = i->punktySuma + j->sumaPunktowLitery;
		}
		i->punktySuma = i->punktySuma * i->premiaWyrazowa;
		std::cout << i->punktySuma << std::endl;
	}
}

void wyswietlNajlepiejPunktowane(cv::Mat obrazek, std::vector<slowo>slowaNaPlanszy)
{
	int max = 0;
	for (auto i = slowaNaPlanszy.begin(); i != slowaNaPlanszy.end(); i++)
	{
		if (i->punktySuma > max)max = i->punktySuma;
	}
	for (auto i = slowaNaPlanszy.begin(); i != slowaNaPlanszy.end(); i++)
	{
		if (i->punktySuma == max)
		{
			cv::rectangle(obrazek, cv::Rect(cv::Point2i(i->litery.begin()->wsp.x - 40, i->litery.begin()->wsp.y - 40), cv::Point2i(i->litery[i->litery.size() - 1].wsp.x + 40, i->litery[i->litery.size() - 1].wsp.y + 40)), cv::Scalar{ 0,0,255 }, 5, 8, 0);
			std::cout << "Wartosc punktowa slowa: " << i->punktySuma << std::endl;
			for (auto j = i->litery.begin(); j != i->litery.end(); j++)
			{
				std::cout << j->recChar << " wartosc z uwzgledniona premia: " <<j->sumaPunktowLitery<<std::endl;
			}
			std::cout << "premia slowna: " << i->premiaWyrazowa << std::endl << std::endl;
		}
	}
	cv::namedWindow("Najlepiej punktowane s�owo", cv::WINDOW_NORMAL);
	cv::imshow( "Najlepiej punktowane s�owo", obrazek);
	cv::waitKey(0);
}

int main()
{

	//nie dzia�a wektor
	if (1 == 1)std::cout << "1";
	//Przygotowanie zdj�cia

	cv::Mat image, tresh;
	image = cv::imread("C:\\Users\\kaczj_000\\OneDrive\\STUDIA\\CPO cyfrowe przetwarzanie obrazu\\CPO_Jakub_Kaczorowski\\JakubKaczorowski41\\ideal.png");
	std::vector<cv::Mat>RGB, RGB2;
	cv::split(image, RGB);


	cv::threshold(RGB[2] - 0.5*RGB[1] - 0.5*RGB[0], tresh, 0, 255, 0);

	cv::erode(tresh, tresh, cv::Mat());
	cv::erode(tresh, tresh, cv::Mat());
	cv::erode(tresh, tresh, cv::Mat());
	cv::erode(tresh, tresh, cv::Mat());

	cv::dilate(tresh, tresh, cv::Mat());
	cv::dilate(tresh, tresh, cv::Mat());
	cv::dilate(tresh, tresh, cv::Mat());
	cv::dilate(tresh, tresh, cv::Mat());




	//Korekcja perspektywy

	std::vector<cv::Point2f> znalezioneNarozniki;
	std::vector<cv::Point2f> doceloweNarozniki = { cv::Point(0, 0) ,cv::Point(0, tresh.rows),cv::Point(tresh.cols, 0),cv::Point(tresh.cols,tresh.rows) };
	znalezioneNarozniki.resize(4);
	znalezioneNarozniki[0] = odleglosc(doceloweNarozniki[0], tresh);
	znalezioneNarozniki[1] = odleglosc(doceloweNarozniki[1], tresh);
	znalezioneNarozniki[2] = odleglosc(doceloweNarozniki[2], tresh);
	znalezioneNarozniki[3] = odleglosc(doceloweNarozniki[3], tresh);

	cv::Mat transf = cv::getPerspectiveTransform(znalezioneNarozniki, doceloweNarozniki);
	cv::warpPerspective(image, image, transf, image.size());

	cv::Mat prosty = image;




	//Wyszukanie kontur�w

	cv::split(image, RGB2);
	cv::threshold(RGB2[1] - 0.55*RGB2[0] + 0.45*RGB2[2], image, 210, 255, 0);

	cv::Mat canny;
	cv::Canny(image, canny, 100, 255, 3, 0);
	cv::threshold(canny, canny, 200, 255, 0);

	std::vector<std::vector<cv::Point> >conturs, conturs2, contursTemp;
	std::vector<cv::Vec4i> hierarhy;
	cv::findContours(canny, conturs, hierarhy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));




	//Filtracja kontur�w

	conturs2 = filterArea(conturs,canny);          //Filtracja z wykorzystaniem pola kontur�w
	conturs2 = filterRectangle(conturs2, canny);   //Filtracja z wykorzystaniem prostokat�w

	//Utworzenie wektora liter
	std::vector<letter>letters;
	letters.resize(conturs2.size());
	for (int i = 0; i < conturs2.size(); i++)
	{
		letters[i].moment = cv::moments(conturs2[i], false);
		letters[i].wsp = cv::Point2f(letters[i].moment.m10 / letters[i].moment.m00, letters[i].moment.m01 / letters[i].moment.m00);
	}


	letters = filterDup(letters,conturs2);         //Filtracja powtarzaj�cyh sie kontur�w

	//Rysowanie wszystkich liter
	cv::Mat drawing = prosty;
	for (int i = 0; i < letters.size(); i++)
	{
		cv::Scalar color = cv::Scalar(255, 255, 255);
		drawContours(drawing, conturs2, i, color, 3, 8, hierarhy, 0, cv::Point());
		cv::circle(drawing, letters[i].wsp, 6, cv::Scalar(255, 140, 130), -1);
	}

	//Mapa do przechowywania warto�ci moment�w z bazy
	std::multimap<char, std::vector<double>>base = loadBase("baza.txt");




	//Rozpoznanie liter

	//Rysowanie kolejnych kontur�w liter na ciemnym polu
	for (int o = 0; o < letters.size(); o++)
	{
	//drawing = cv::Mat::zeros(canny.size(), CV_8UC3);
	//drawContours(drawing, conturs2, o, cv::Scalar(255, 255, 255), 3, 8, hierarhy, 0, cv::Point());
	//cv::circle(drawing, letters[o].wsp, 6, cv::Scalar(255, 140, 130), -1);

	std::cout << recogniseLetter(letters[o], base) << letters[o].wsp.x << " " << letters[o].wsp.y << std::endl;
	std::cout << letters[o].moment.m00 << std::endl << letters[o].moment.m01 << std::endl << letters[o].moment.m02 << std::endl << letters[o].moment.m03 << std::endl << letters[o].moment.m10 << std::endl << letters[o].moment.m11 << std::endl << letters[o].moment.m12 << std::endl;
	std::cout << std::endl;
	//cv::imshow("test", drawing);
	//cv::waitKey(0);
	}




	//Obliczanie punkt�w za s�owa

	int premieLiterowe[15][15];        //Warto�ci premii s�ownych dla ka�dgo pola planszy
	int premieWyrazowe[15][15];        //Warto�ci premii wyrazowych dla ka�dego pola planszy
	letter literyNaPlanszy[15][15];    //Po�o�enie rozpoznanych liter na planszy
	std::map<char, int>wartosciLiter;  //Do przechowywania warto�ci punktowej litery


	wczytajPremie(premieLiterowe, "premieLiterowe.txt");
	wczytajPremie(premieWyrazowe, "premieWyrazowe.txt");
	wartosciLiter =  wczytajWartosciLiter("wartosci.txt");


	zamienWspolrzedneNaPlansze(letters, literyNaPlanszy, canny);
	wyswietlLiteryNaPlanszy(literyNaPlanszy);

	uzupelnijPunktacjeLiter(literyNaPlanszy,premieLiterowe,premieWyrazowe,wartosciLiter);


	std::vector<slowo>slowaNaPlanszy = znajdzSlowaNaPlanszy(literyNaPlanszy);
	slowaNaPlanszy=filtrujS�owaNaPlanszy(slowaNaPlanszy);
	wyswietSlowa(slowaNaPlanszy);

	policzPunktySlowNaPlanszy(slowaNaPlanszy);
	wyswietlNajlepiejPunktowane(drawing, slowaNaPlanszy);


	std::cin.get();
}
