#include "stdafx.h"
#include <vector>

template <typename TPixelType>
struct Image
{
  typedef TPixelType PixelType;

  int width;
  int height;
  std::vector<PixelType> m_data;
};

template <typename ImageType>
struct Map
{
  int width;
  int height;
  typename ImageType::PixelType* m_data;
};

int main(int argc, char* argv[])
{
	vt::CLumaFloatImg a(20,3);



  Image<float> img;
  img.width = 2;
  img.height = 2;
  img.m_data.push_back(1);
  img.m_data.push_back(2);
  img.m_data.push_back(3);
  img.m_data.push_back(4);

  Map< Image<float> > map_img;
  map_img.width = img.width;
  map_img.height = img.height;
  map_img.m_data = img.m_data.data();

  std::vector<std::vector<std::vector<char>>> asdf;

  int halt = 0;
}