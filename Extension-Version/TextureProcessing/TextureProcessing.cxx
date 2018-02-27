#include "itkPluginUtilities.h"
#include "TextureProcessingCLP.h"

#include "Global.h"

#include "itkImageFileWriter.h"
#include <itkImageDuplicator.h>
#include <itkCastImageFilter.h>
#include "itkN4BiasFieldCorrectionImageFilter.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkBinaryBallStructuringElement.h"

#include "Plot/HistogramPlot.h"
#include "Percents/PercentileCalc.h"
#include "Features/HistoFeat.h"
#include "CoocurrenceFeat/CoocurrenceFeat.h"
#include "ShapeFeatures/ShapeFeat.h"
#include "RunLengthFeat/RunLengthFeat.h"
#include "Normalization/Normalize.h"
#include "Gradient/GradientFeat.h"

//#include <iostream>
//#include <sstream>
//#include <ofstream.h>
#include <sys/stat.h>

// Use an anonymous namespace to keep class types and function names
// from colliding when module is used as shared object module.  Every
// thing should be in an anonymous namespace except for the module
// entry point, e.g. main()

namespace
{

template <class T>
int DoIt( int argc, char * argv[], T )
{
    PARSE_ARGS;

    if( argc < 3 )
    {
        std::cerr << "Missing command line arguments" << std::endl;
        std::cerr << "Usage :  Image  inputImageFileName " << std::endl;
        return EXIT_FAILURE;
    }

    typedef    T InputPixelType;

    typedef itk::Image<InputPixelType,  Dimension> InputImageType;

    typedef itk::ImageFileReader<InputImageType>  ReaderType;

    std::cout << "Etapa 1 - Reading" <<std::endl;

    // Instantiating and filling inputs
    typename ReaderType::Pointer imageReader = ReaderType::New();
    imageReader->SetFileName( inputVolume.c_str() );
    imageReader->Update();

    typename ReaderType::Pointer labelReader = ReaderType::New();
    labelReader->SetFileName( inputLabel.c_str() );
    labelReader->Update();

    /*

    // ###############
    // catching filename (with extension) to use in saving file routine
    std::string fullname = inputVolume.c_str();
    std::string fileNameWithExtension = fullname.substr(fullname.find_last_of("\\/")+1);
    // Removing extension
    std::string fileName = fileNameWithExtension.substr(0, fileNameWithExtension.find_last_of("."));
    //std::cout <<  " Name "<<fileName <<std::endl;
    // ###############

    std::cout << "Etapa 3 - Pre-processing" <<std::endl;
    // ###################################################################################
    //* Pre processing.

    // Cast short Filter over feature image
    typedef itk::CastImageFilter<InputImageType, InternalImageType> CastFilterType;
    typename CastFilterType::Pointer labelCastFilter = CastFilterType::New();
    labelCastFilter->SetInput(labelReader->GetOutput());
    labelCastFilter->Update();

    // Defining Eroding parameter (Structuring Element)
    int radius = 8;
    typedef itk::BinaryBallStructuringElement<InternalPixelType,Dimension> StructuringElementType;
    StructuringElementType structuringElement;
    structuringElement.SetRadius(radius);
    structuringElement.CreateStructuringElement();

    // Dilating mask for bIAS correction
    typedef itk::BinaryDilateImageFilter <InternalImageType, InternalImageType, StructuringElementType>
            BinaryDilateImageFilterType;
    typename BinaryDilateImageFilterType::Pointer dilateFilter = BinaryDilateImageFilterType::New();
    dilateFilter->SetInput(labelCastFilter->GetOutput());
    dilateFilter->SetKernel(structuringElement);

    // N4 Bias Field Correction
    typedef itk::N4BiasFieldCorrectionImageFilter<InputImageType,InternalImageType> CorrectionFilterType;
    typename CorrectionFilterType::Pointer correctionFilter = CorrectionFilterType::New();
    correctionFilter->SetInput(imageReader->GetOutput());
    correctionFilter->SetMaskImage(dilateFilter->GetOutput());

    try
    {
        correctionFilter->Update();
    }
    catch( itk::ExceptionObject & excp )
    {
        std::cerr << "Problem correcting image image file : " << argv[1] << std::endl;
        std::cerr << excp << std::endl;
        return EXIT_FAILURE;
    }

    // Cast short Filter over feature image
    typename CastFilterType::Pointer imageCastFilter = CastFilterType::New();
    imageCastFilter->SetInput(correctionFilter->GetOutput());
    imageCastFilter->Update();

    typename InternalImageType::Pointer featureImage;
    typename InternalImageType::Pointer labelImage;

    //* End of pre processing.
    // ###################################################################################

    std::cout << "Etapa 2 - Normalization" <<std::endl;
    // ###################################################################################
    // **Calling normalization Routine

    if(doNormalization){
        Normalize* normalize = new Normalize(imageCastFilter->GetOutput(),labelCastFilter->GetOutput());
        normalize->Run();
        // Grayscale Image
        featureImage = normalize->GetNormalized(0);
        // Label Image
        labelImage = normalize->GetNormalized(1);

        std::cout << "Etapa ** - Normalized" <<std::endl;

    } else{
        // If it's chosen not to normalize pixel intensities, it's given the original image.
        featureImage = imageCastFilter->GetOutput();
        labelImage = labelCastFilter->GetOutput();

        std::cout << "Etapa **** - NOT Normalized" <<std::endl;
    }

    std::cout << "Etapa 4 - Path Configuration" <<std::endl;
    // ###################################################################################
    //** Configuring saving path!

    struct stat sb;
    stringstream temp;

    // Checks if spath /TextureProcessing alredy exists. If not, creates it.
    spath = spath+"/TextureProcessing";       // Updates My spath Variable!
    temp.str(std::string());

    // if spath/Textureprocessing doesn't exist, creates it!
    if (!stat(spath.c_str(), &sb) == 0 || !S_ISDIR(sb.st_mode)){
        temp<<"mkdir "<<spath;
        system(temp.str().c_str());
    }

    // Combining the strings into one unique string variable to create the fileName.csv file.
    std::string FileHistoStat = spath + "/" + fileName + ".csv";
    // std::string FileHistoStat = fileName + ".csv";
    const char * fHS = FileHistoStat.c_str();
    std::ofstream features( fHS );

    // Combining the strings into one unique string variable to create the Histogram.txt file.
    std::string FileHisto = spath + "/Histogram.txt";
    const char * fH = FileHisto.c_str();
    std::ofstream histo( fH );

    // Combining the strings into one unique string variable to create the Percents.txt file.
    std::string FilePerc = spath + "/Percents.txt";
    const char * fP = FilePerc.c_str();
    std::ofstream percents( fP );

    // End of path configuration
    // ###################################################################################


    std::cout << "Etapa 5 - Calling Functions" <<std::endl;
    // ###################################################################################
    //** Calling processing functions

    // Calling the plotting function
    HistogramPlot* histogram = new HistogramPlot( featureImage, labelImage);
    histogram->Run();

    // Writing features headers into fileNma.csv file
    features<< ",Percent 5th,Percent 10th,Percent 25th,Percent 75th,Percent 90th,Quartile,GL Entropy,"<<
               "Mean,Std Deviation,Median,Skewness,Kurtosis,Variance,Histogram Elongation,Flatness,Minimum,Maximum,"<<
               "Entropy,Energy,Correlation,Haralick Correlation,Cluster Proeminence,Cluster Shade,Inertia,Inverse Difference Moment,"<<
               "SRE,LRE,GLN,RLN,LGRE,HGRE,SRLGE,SRHGE,LRLGE,LRHGE,RP,"<<
               "Volume In Physical Units,Size Ratio,Elongation,Feret Diameter,Equiv Spher Perimeter,Equiv Spher Radius,Perimeter,Roundness,"<<
               "Gradient Mean,Gradient Std,Gradient Variance,Gradient Maximum"<<endl;
    features<<fileName<<",";

    // Receiving the frequecy container vector;
    int freq[histoSize];
    for (int i = 0; i<histoSize; i++){
        freq[i] = histogram->GetHistogram(i);
    }

    if(doHistoPlot){
        // Histogram

        histo << "#Histogram size " << histoSize << std::endl;
        histo << "#bin   frequency" << std::endl;
        for ( int bin=0; bin < histoSize; ++bin ) {
            histo << bin << "    " << histogram->GetHistogram(bin)<< std::endl;
        }
        histo.close();
    }

    if(doPercentiles){
        PercentileCalc* percentiles = new PercentileCalc( freq, spath);
        percentiles->Run();

        // Percents
        percents << "#Percents " << histoSize << std::endl;
        percents << "#th   percentile bin" << std::endl;
        for ( int p=1; p < 101; ++p ) {
            percents << p << "th    :" << percentiles->GetPercentiles(p)<< std::endl;
        }
        percents.close();

        // Specific Percentis Features
        features<< percentiles->GetPercentiles(5)<<","<<percentiles->GetPercentiles(10)<<","
                <<percentiles->GetPercentiles(25)<<","<<percentiles->GetPercentiles(75)<<","
               <<percentiles->GetPercentiles(90)<<","
              <<abs(percentiles->GetPercentiles(75) - percentiles->GetPercentiles(25))<<","
             <<percentiles->GetPercentiles(0)<<",";

        std::cout << "**Percentile - ok" <<std::endl;
    }

    if(doHistogramFeat){
        HistoFeat* histoFeatures = new HistoFeat(featureImage, labelImage);
        histoFeatures->Run();

        // Histo Features
        features<<histoFeatures->GetFeatures(0)<<","<<histoFeatures->GetFeatures(1)<<","<<histoFeatures->GetFeatures(2)
               <<","<<histoFeatures->GetFeatures(3)<<","<<histoFeatures->GetFeatures(4)<<","<<histoFeatures->GetFeatures(6)
              <<","<<histoFeatures->GetFeatures(7)<<","<<histoFeatures->GetFeatures(8)<<","<<histoFeatures->GetFeatures(9)
             <<","<<histoFeatures->GetFeatures(10)<<",";

        std::cout << "**Histogram Features - ok" <<std::endl;
    }

    if(doCoocurrenceFeat){
        CoocurrenceFeat* coocurrenceFeat = new CoocurrenceFeat(featureImage, labelImage);
        coocurrenceFeat->Run();

        // Coocurrence Features
        features<<coocurrenceFeat->GetFeatures(0)<<","<<coocurrenceFeat->GetFeatures(1)<<","<<coocurrenceFeat->GetFeatures(2)
               <<","<<coocurrenceFeat->GetFeatures(3)<<","<<coocurrenceFeat->GetFeatures(4)
              <<","<<coocurrenceFeat->GetFeatures(5)<<","<<coocurrenceFeat->GetFeatures(6)
             <<","<<coocurrenceFeat->GetFeatures(7)<<",";

        std::cout << "**Coocurrence Features - ok" <<std::endl;
    }

    if(doRunFeat){
        RunLengthFeat* runLengthFeat = new RunLengthFeat(featureImage, labelImage);
        runLengthFeat->Run();

        // RunLength Features
        features<<runLengthFeat->GetFeatures(0)<<","<<runLengthFeat->GetFeatures(1)
               <<","<<runLengthFeat->GetFeatures(2)<<","<<runLengthFeat->GetFeatures(3)
              <<","<<runLengthFeat->GetFeatures(4)<<","<<runLengthFeat->GetFeatures(5)
             <<","<<runLengthFeat->GetFeatures(6)<<","<<runLengthFeat->GetFeatures(7)
            <<","<<runLengthFeat->GetFeatures(8)<<","<<runLengthFeat->GetFeatures(9)
           <<","<<runLengthFeat->GetFeatures(10)<<",";

        std::cout << "**RunLength Features - ok" <<std::endl;
    }

    /*if(doShapeFeat){
        ShapeFeat* shapeFeatures = new ShapeFeat(labelImage);
        shapeFeatures->Run();

        // Shape Features
        features<<shapeFeatures->GetFeatures(1)<<","<<shapeFeatures->GetFeatures(2)
               <<","<<shapeFeatures->GetFeatures(3)<<","<<shapeFeatures->GetFeatures(4)
              <<","<<shapeFeatures->GetFeatures(5)<<","<<shapeFeatures->GetFeatures(6)
             <<","<<shapeFeatures->GetFeatures(7)<<","<<shapeFeatures->GetFeatures(8)<<",";

        std::cout << "**Shape Features - ok" <<std::endl;
    }*/

    /*

    if(doGradient){
        GradientFeat* gradientFeatures = new GradientFeat(featureImage, labelImage);
        gradientFeatures->Run();

        //Gradient Features
        features<<gradientFeatures->GetFeatures(0)<<","<<gradientFeatures->GetFeatures(1)
               <<","<<gradientFeatures->GetFeatures(2)<<","<<gradientFeatures->GetFeatures(3);

        std::cout << "**Gradient Features - ok" <<std::endl;
    }

    features.close();

    //** End Calling Processing functions
    // ###################################################################################

    */
    /*
  // Saving the Images
    typedef itk::ImageFileWriter<InternalImageType>  WriterType;
    typename WriterType::Pointer imageWriter = WriterType::New();

    imageWriter->SetInput( labelImage );
    std::string imagePath1 = spath + "/NormlabelImage.nrrd";
    //const char * fP1 = imagePath1.c_str();

    imageWriter->SetFileName(imagePath1);
    imageWriter->Update();

    imageWriter->SetInput( featureImage );
    std::string imagePath2 = spath + "/NormfeatureImage.nrrd";
    //const char * fP2 = imagePath2.c_str();
    imageWriter->SetFileName(imagePath2);
    imageWriter->Update();
  */

    std::cout << "End execution!" <<std::endl;
    // ###################################################################################

    return EXIT_SUCCESS;
}

} // end of anonymous namespace

int main( int argc, char * argv[] )
{
    PARSE_ARGS;

    itk::ImageIOBase::IOPixelType     pixelType;
    itk::ImageIOBase::IOComponentType componentType;

    try
    {
        itk::GetImageType(inputVolume, pixelType, componentType);

        // This filter handles all types on input, but only produces
        // signed types
        switch( componentType )
        {
        case itk::ImageIOBase::UCHAR:
            return DoIt( argc, argv, static_cast<unsigned char>(0) );
            break;
        case itk::ImageIOBase::CHAR:
            return DoIt( argc, argv, static_cast<char>(0) );
            break;
        case itk::ImageIOBase::USHORT:
            return DoIt( argc, argv, static_cast<unsigned short>(0) );
            break;
        case itk::ImageIOBase::SHORT:
            return DoIt( argc, argv, static_cast<short>(0) );
            break;
        case itk::ImageIOBase::UINT:
            return DoIt( argc, argv, static_cast<unsigned int>(0) );
            break;
        case itk::ImageIOBase::INT:
            return DoIt( argc, argv, static_cast<int>(0) );
            break;
        case itk::ImageIOBase::ULONG:
            return DoIt( argc, argv, static_cast<unsigned long>(0) );
            break;
        case itk::ImageIOBase::LONG:
            return DoIt( argc, argv, static_cast<long>(0) );
            break;
        case itk::ImageIOBase::FLOAT:
            return DoIt( argc, argv, static_cast<float>(0) );
            break;
        case itk::ImageIOBase::DOUBLE:
            return DoIt( argc, argv, static_cast<double>(0) );
            break;
        case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
        default:
            std::cout << "unknown component type" << std::endl;
            break;
        }
    }

    catch( itk::ExceptionObject & excep )
    {
        std::cerr << argv[0] << ": exception caught !" << std::endl;
        std::cerr << excep << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
