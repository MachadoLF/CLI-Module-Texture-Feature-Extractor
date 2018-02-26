# CLI-Module---Texture-Feature-Extractor
This repository contain both the Extension (Created and edited with Extension Wizard tool) and Independent CLI module (Created directly containing a single CMakeLists.txt file) version.

The Independent CLI version is compiling with no single error. Although the Extension version carries the same files, definitions and so on, it show some critical error:

error: undefined reference to `itk::GeometryUtilities::HyperSphereRadiusFromVolume(int, double)’
/usr/local/Slicer/ITKv4/Modules/Filtering/LabelMap/include/itkShapeLabelMapFilter.hxx:372:

error: undefined reference to `itk::GeometryUtilities::HyperSpherePerimeter(int, double)’
/usr/local/Slicer/ITKv4/Modules/Filtering/LabelMap/include/itkShapeLabelMapFilter.hxx:373:

error: undefined reference to `itk::GeometryUtilities::HyperSphereRadiusFromVolume(int, double)’
/usr/local/Slicer/ITKv4/Modules/Filtering/LabelMap/include/itkShapeLabelMapFilter.hxx:372:

error: undefined reference to `itk::GeometryUtilities::HyperSpherePerimeter(int, double)’
/usr/local/Slicer/ITKv4/Modules/Filtering/LabelMap/include/itkShapeLabelMapFilter.hxx:373:’
