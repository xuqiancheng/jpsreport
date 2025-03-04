/**
 * \file        Method_I.cpp
 * \date        Feb 07, 2019
 * \version     v0.8
 * \copyright   <2009-2019> Forschungszentrum Jülich GmbH. All rights reserved.
 *
 * \section License
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 *
 * \section Description
 * In this file functions related to method I are defined.
 *
 *
 **/

#include "Method_I.h"
#include <cmath>
#include<map>
#include<iostream>
#include<vector>
#include <tuple>
//using std::string;
//using std::vector;
//using std::ofstream;
using namespace std;



Method_I::Method_I()
{
     _grid_size_X = 0.10;
     _grid_size_Y = 0.10;
     _fps=16;
     _outputVoronoiCellData = false;
     _getProfile = false;
     _geoMinX = 0;
     _geoMinY = 0;
     _geoMaxX = 0;
     _geoMaxY = 0;
     _cutByCircle = false;
     _cutRadius = -1;
     _circleEdges = -1;
     _fIndividualFD = nullptr;
     _calcIndividualFD = true;
     _areaForMethod_I = nullptr;
     _plotVoronoiCellData=false;
     _isOneDimensional=false;
     _startFrame =-1;
     _stopFrame = -1;
}

Method_I::~Method_I()
{

}
// auto outOfRange = [&](int number, int start, int end) -> bool
// {
//      return (number < start || number > end);
// };
bool Method_I::Process(const PedData& peddata,const fs::path& scriptsLocation, const double& zPos_measureArea)
{
     bool return_value = true;
     _scriptsLocation = scriptsLocation;
     _outputLocation = peddata.GetOutputLocation();
     _peds_t = peddata.GetPedsFrame();
     _trajName = peddata.GetTrajName();
     _projectRootDir = peddata.GetProjectRootDir();
     _measureAreaId = boost::lexical_cast<string>(_areaForMethod_I->_id);
     _fps =peddata.GetFps();
     int mycounter = 0;
     int minFrame = peddata.GetMinFrame();
     Log->Write("INFO:\tMethod I: frame rate fps: <%.2f>, start: <%d>, stop: <%d> (minFrame = %d)", _fps, _startFrame, _stopFrame, minFrame);
     if(_startFrame != _stopFrame)
     {
          if(_startFrame==-1)
          {
               _startFrame = minFrame;
          }
          if(_stopFrame==-1)
          {
               _stopFrame = peddata.GetNumFrames()+minFrame;
          }
          for(std::map<int , std::vector<int> >::iterator ite=_peds_t.begin();ite!=_peds_t.end();)
          {
               if((ite->first + minFrame)<_startFrame || (ite->first + minFrame) >_stopFrame)
               {
                    mycounter++;
                    ite = _peds_t.erase(ite);
               }
               else
               {
                    ++ite;
               }


          }
     }

     if(_calcIndividualFD)
     {
          if (!OpenFileIndividualFD())
          {
               return_value = false;
          }
     }
     Log->Write("------------------------ Analyzing with Method I -----------------------------");
     //for(int frameNr = 0; frameNr < peddata.GetNumFrames(); frameNr++ )
     //for(std::map<int , std::vector<int> >::iterator ite=_peds_t.begin();ite!=_peds_t.end();ite++)

     for(auto ite: _peds_t)
     {
          int frameNr = ite.first;
          int frid =  frameNr + minFrame;
          //padd the frameid with 0
          std::ostringstream ss;
          ss << std::setw(5) << std::setfill('0') << std::internal << frid;
          const std::string str_frid = ss.str();
          if(!(frid%50))
          {
               Log->Write("INFO:\tframe ID = %d",frid);
          }
          vector<int> ids=_peds_t[frameNr];
          vector<int> IdInFrame = peddata.GetIdInFrame(frameNr, ids, zPos_measureArea);
          vector<double> XInFrame = peddata.GetXInFrame(frameNr, ids, zPos_measureArea);
          vector<double> YInFrame = peddata.GetYInFrame(frameNr, ids, zPos_measureArea);
          vector<double> ZInFrame = peddata.GetZInFrame(frameNr, ids, zPos_measureArea);
          vector<double> VInFrame = peddata.GetVInFrame(frameNr, ids, zPos_measureArea);
          if(XInFrame.size() == 0)
          {
               Log->Write("Warning:\t no pedestrians in frame <%d>", frameNr);
               continue;
          }
          //vector int to_remove
          //------------------------------Remove peds outside geometry------------------------------------------
          for( int i=0;i<(int)IdInFrame.size();i++)
          {
               if(false==within(point_2d(round(XInFrame[i]), round(YInFrame[i])), _geoPoly))
               {
                    Log->Write("Warning:\tPedestrian at <x=%.4f, y=%.4f, , z=%.4f> is not in geometry and not considered in analysis!", XInFrame[i]*CMtoM, YInFrame[i]*CMtoM, ZInFrame[i]*CMtoM );
                    IdInFrame.erase(IdInFrame.begin() + i);
                    XInFrame.erase(XInFrame.begin() + i);
                    YInFrame.erase(YInFrame.begin() + i);
                    ZInFrame.erase(ZInFrame.begin() + i);
                    VInFrame.erase(VInFrame.begin() + i);
                    Log->Write("Warning:\t Pedestrian removed");
                    i--;

               }
          }
          // int NumPeds = IdInFrame.size();
          // std::cout << "numpeds = " << NumPeds << "\n";

//---------------------------------------------------------------------------------------------------------------
          if(_isOneDimensional)
          {
               CalcVoronoiResults1D(XInFrame, VInFrame, IdInFrame, _areaForMethod_I->_poly,str_frid);
          }
          else
          {
               if(IsPointsOnOneLine(XInFrame, YInFrame))
               {
                    if(fabs(XInFrame[1]-XInFrame[0])<dmin)
                    {
                         XInFrame[1]+= offset;
                    }
                    else
                    {
                         YInFrame[1]+= offset;
                    }
               }
               std::vector<std::pair<polygon_2d, int> > polygons_id = GetPolygons(XInFrame, YInFrame, VInFrame, IdInFrame);
               // std::cout << ">> polygons_id " << polygons_id.size() << "\n";
               vector<polygon_2d> polygons;
               for (auto p: polygons_id)
               {
                    polygons.push_back(p.first);
          }

               if(!polygons.empty())
               {
                    if(_calcIndividualFD)
                    {
                         if(!_isOneDimensional)
                         {
                              // GetIndividualFD(polygons,VInFrame, IdInFrame,  str_frid); // TODO polygons_id
                              GetIndividualFD(polygons,VInFrame, IdInFrame,  str_frid, XInFrame, YInFrame, ZInFrame); //
                         }
                    }
                    // ToDo: is obsolete
//                    if(_getProfile)
//                    { //	field analysis
//                         GetProfiles(str_frid, polygons, VInFrame); // TODO polygons_id
//                    }
                    // ToDo: is obsolete
//                    if(_outputVoronoiCellData)
//                     { // output the Voronoi polygons of a frame
//                         OutputVoroGraph(str_frid, polygons_id, NumPeds,VInFrame);
//                     }
               }
               else
               {
                    for(int i=0;i<(int)IdInFrame.size();i++)
                    {
                         std::cout << XInFrame[i]*CMtoM << "   " << YInFrame[i]*CMtoM <<  "   "  << IdInFrame[i] << "\n";
                    }
                    Log->Write("WARNING: \tVoronoi Diagrams are not obtained!. Frame: %d (minFrame = %d)\n", frid, minFrame);
               }
          }
     }//peds
     if(_calcIndividualFD)
     {
          fclose(_fIndividualFD);
     }
    return return_value;
}

// ToDo: this function OpenFileMethodI is obsolete - can be deleted

//     bool Method_I::OpenFileMethodI()
//     {
//
//          std::string voroLocation(VORO_LOCATION);
//          fs::path tmp("_id_"+_measureAreaId+".dat");
//          tmp =  _outputLocation / voroLocation / ("rho_v_Voronoi_" + _trajName.string() + tmp.string());
//          // _outputLocation.string() +  voroLocation+"rho_v_Voronoi_"+_trajName+"_id_"+_measureAreaId+".dat";
//          string results_V= tmp.string();
//
//
//          if((_fVoronoiRhoV=Analysis::CreateFile(results_V))==nullptr)
//          {
//               Log->Write("ERROR: \tcannot open the file to write Voronoi density and velocity\n");
//               return false;
//          }
//          else
//          {
//               if(_isOneDimensional)
//               {
//                    fprintf(_fVoronoiRhoV,"#framerate:\t%.2f\n\n#Frame \t Voronoi density(m^(-1))\t	Voronoi velocity(m/s)\n",_fps);
//               }
//               else
//               {
//                    fprintf(_fVoronoiRhoV,"#framerate:\t%.2f\n\n#Frame \t Voronoi density(m^(-2))\t	Voronoi velocity(m/s)\n",_fps);
//               }
//               return true;
//          }
//     }



     bool Method_I::OpenFileIndividualFD()
     {
          fs::path trajFileName("_id_"+_measureAreaId+".dat");
          fs::path indFDPath("Fundamental_Diagram");
          indFDPath = _outputLocation / indFDPath / "IndividualFD" / ("IFD_I_" +_trajName.string() + trajFileName.string());
          string Individualfundment=indFDPath.string();
          if((_fIndividualFD=Analysis::CreateFile(Individualfundment))==nullptr)
          {
               Log->Write("ERROR:\tcannot open the file individual\n");
               return false;
          }
          else
          {
               if(_isOneDimensional)
               {
                    fprintf(_fIndividualFD,"#framerate (fps):\t%.2f\n\n#Frame\tPersID\tIndividual density(m^(-1))\tIndividual velocity(m/s)\tHeadway(m)\n",_fps);
               }
               else
               {
                    fprintf(_fIndividualFD,"#framerate (fps):\t%.2f\n\n#Frame\tPersID\tx/m\ty/m\tz/m\tIndividual density(m^(-2))\tIndividual velocity(m/s)\tVoronoi Polygon\n",_fps);
               }
               return true;
          }
     }

     std::vector<std::pair<polygon_2d, int> > Method_I::GetPolygons(vector<double>& XInFrame, vector<double>& YInFrame, vector<double>& VInFrame, vector<int>& IdInFrame)
     {
          VoronoiDiagram vd;
          //int NrInFrm = ids.size();
          double boundpoint =10*max(max(fabs(_geoMinX),fabs(_geoMinY)), max(fabs(_geoMaxX), fabs(_geoMaxY)));
          std::vector<std::pair<polygon_2d, int> > polygons_id;
          polygons_id = vd.getVoronoiPolygons(XInFrame, YInFrame, VInFrame,IdInFrame, boundpoint);
          polygon_2d poly ;
          if(_cutByCircle)
          {
               polygons_id = vd.cutPolygonsWithCircle(polygons_id, XInFrame, YInFrame, _cutRadius,_circleEdges);
          }
//todo HH
          polygons_id = vd.cutPolygonsWithGeometry(polygons_id, _geoPoly, XInFrame, YInFrame);
          // todo HH
          // std:: cout  << dsv(_geoPoly) << "\n"; // ToDo: obsolete ?
          for(auto && p:polygons_id)
          {
               poly = p.first;
               ReducePrecision(poly);
               // TODO update polygon_id?
          }
          // std:: cout << " GetPolygons leave " << polygons_id.size() << "\n"; // ToDo: obsolete ?
          return polygons_id;
     }
/**
 * Output the Voronoi density and velocity in the corresponding file
 */

// ToDo: this function OutputVoronoiResults is obsolete - can be deleted

//     void Method_I::OutputVoronoiResults(const vector<polygon_2d>&  polygons, const string& frid, const vector<double>& VInFrame)
//     {
//          double VoronoiVelocity=1;
//          double VoronoiDensity=-1;
//          std::tie(VoronoiDensity, VoronoiVelocity) = GetVoronoiDensityVelocity(polygons,VInFrame,_areaForMethod_I->_poly);
//          fprintf(_fVoronoiRhoV,"%s\t%.3f\t%.3f\n",frid.c_str(),VoronoiDensity, VoronoiVelocity);
//     }

/*
 * calculate the voronoi density and velocity according to voronoi cell of each pedestrian and their instantaneous velocity "Velocity".
 * input: voronoi cell and velocity of each pedestrian and the measurement area
 * output: the voronoi density and velocity in the measurement area (tuple)
 */

// ToDo: this function GetVoronoiDensityVelocity obsolete - can be deleted

//     std::tuple<double,double> Method_I::GetVoronoiDensityVelocity(const vector<polygon_2d>& polygon, const vector<double>& Velocity, const polygon_2d & measureArea)
//     {
//          double meanV=0;
//          double density=0;
//          int temp=0;
//          for (auto && polygon_iterator:polygon)
//          {
//               polygon_list v;
//               intersection(measureArea, polygon_iterator, v);
//               if(!v.empty())
//               {
//                    meanV+=Velocity[temp]*area(v[0]);
//                    density+=area(v[0])/area(polygon_iterator);
//                    if((area(v[0]) - area(polygon_iterator))>J_EPS)
//                    {
//                         std::cout<<"----------------------Now calculating density-velocity!!!-----------------\n ";
//                         std::cout<<"measure area: \t"<<std::setprecision(16)<<dsv(measureArea)<<"\n";
//                         std::cout<<"Original polygon:\t"<<std::setprecision(16)<<dsv(polygon_iterator)<<"\n";
//                         std::cout<<"intersected polygon: \t"<<std::setprecision(16)<<dsv(v[0])<<"\n";
//                         std::cout<<"this is a wrong result in density calculation\t "<<area(v[0])<<'\t'<<area(polygon_iterator)<<  "  (diff=" << (area(v[0]) - area(polygon_iterator)) << ")" << "\n";
//                    }
//               }
//               temp++;
//          }
//          meanV = meanV/area(measureArea);
//          density = density/(area(measureArea)*CMtoM*CMtoM);
//          return std::make_tuple(density, meanV);
//     }

// ToDo: this function GetProfiles is obsolete - can be deleted
// and velocity is calculated for every frame
//     void Method_I::GetProfiles(const string& frameId, const vector<polygon_2d>& polygons, const vector<double>& velocity)
//     {
//          std::string voroLocation(VORO_LOCATION);
//          fs::path tmp("field");
//          fs::path vtmp ("velocity");
//          fs::path dtmp("density");
//          tmp = _outputLocation / voroLocation / tmp;
//          vtmp = tmp / vtmp / ("Prf_v_"+_trajName.string()+"_id_"+_measureAreaId+"_"+frameId+".dat");
//          dtmp = tmp / dtmp / ("Prf_d_"+_trajName.string()+"_id_"+_measureAreaId+"_"+frameId+".dat");
//          //string voronoiLocation=_outputLocation.string() + voroLocation+"field/";
//
//          // string Prfvelocity=voronoiLocation+"/velocity/Prf_v_"+_trajName.string()+"_id_"+_measureAreaId+"_"+frameId+".dat";
//          // string Prfdensity=voronoiLocation+"/density/Prf_d_"+_trajName.string()+"_id_"+_measureAreaId+"_"+frameId+".dat";
//          string Prfvelocity = vtmp.string();
//          string Prfdensity = dtmp.string();
//
//          FILE *Prf_velocity;
//          if((Prf_velocity=Analysis::CreateFile(Prfvelocity))==nullptr) {
//               Log->Write("cannot open the file <%s> to write the field data\n",Prfvelocity.c_str());
//               exit(EXIT_FAILURE);
//          }
//          FILE *Prf_density;
//          if((Prf_density=Analysis::CreateFile(Prfdensity))==nullptr) {
//               Log->Write("cannot open the file to write the field density\n");
//               exit(EXIT_FAILURE);
//          }
//
//          int NRow = (int)ceil((_geoMaxY-_geoMinY)/_grid_size_Y); // the number of rows that the geometry will be discretized for field analysis
//          int NColumn = (int)ceil((_geoMaxX-_geoMinX)/_grid_size_X); //the number of columns that the geometry will be discretized for field analysis
//          for(int row_i=0; row_i<NRow; row_i++) { //
//               for(int colum_j=0; colum_j<NColumn; colum_j++) {
//                    polygon_2d measurezoneXY;
//                    {
//                         const double coor[][2] = {
//                              {_geoMinX+colum_j*_grid_size_X,_geoMaxY-row_i*_grid_size_Y}, {_geoMinX+colum_j*_grid_size_X+_grid_size_X,_geoMaxY-row_i*_grid_size_Y}, {_geoMinX+colum_j*_grid_size_X+_grid_size_X, _geoMaxY-row_i*_grid_size_Y-_grid_size_Y},
//                              {_geoMinX+colum_j*_grid_size_X, _geoMaxY-row_i*_grid_size_Y-_grid_size_Y},
//                              {_geoMinX+colum_j*_grid_size_X,_geoMaxY-row_i*_grid_size_Y} // closing point is opening point
//                         };
//                         assign_points(measurezoneXY, coor);
//                    }
//                    correct(measurezoneXY);     // Polygons should be closed, and directed clockwise. If you're not sure if that is the case, call this function
//
//                    double densityXY;
//                    double velocityXY;
//                    std::tie(densityXY, velocityXY) = GetVoronoiDensityVelocity(polygons,velocity,measurezoneXY);
//                    fprintf(Prf_density,"%.3f\t",densityXY);
//                    fprintf(Prf_velocity,"%.3f\t",velocityXY);
//               }
//               fprintf(Prf_density,"\n");
//               fprintf(Prf_velocity,"\n");
//          }
//          fclose(Prf_velocity);
//          fclose(Prf_density);
//     }

// ToDo: this function OutputVoroGraph is obsolete - can be deleted

//     void Method_I::OutputVoroGraph(const string & frameId,  std::vector<std::pair<polygon_2d, int> >& polygons_id, int numPedsInFrame,const vector<double>& VInFrame)
//     {
//          //string voronoiLocation=_projectRootDir+"./Output/Fundamental_Diagram/Classical_Voronoi/VoronoiCell/id_"+_measureAreaId;
//
//          fs::path voroLocPath(_outputLocation);
//          fs::path voro_location_path (VORO_LOCATION); //
//          // this MACRO to
//          // path. Maybe
//          // remove the MACRO?
//          voroLocPath = voroLocPath / voro_location_path /  "VoronoiCell";
//          polygon_2d poly;
//          if(!fs::exists(voroLocPath))
//          {
//               if(!fs::create_directories(voroLocPath))
//               {
//                    Log->Write("ERROR:\tcan not create directory <%s>", voroLocPath.string().c_str());
//                    std::cout << "can not create directory "<< voroLocPath.string().c_str() << "\n";
//                    exit(EXIT_FAILURE);
//               }
//               else
//                    std::cout << "create directory "<< voroLocPath.string().c_str() << "\n";
//          }
//
//          fs::path polygonPath=voroLocPath / "polygon";
//          if(!fs::exists(polygonPath))
//          {
//               if(!fs::create_directory(polygonPath))
//               {
//                    Log->Write("ERROR:\tcan not create directory <%s>", polygonPath.string().c_str());
//                    exit(EXIT_FAILURE);
//               }
//          }
//          fs::path trajFileName(_trajName.string()+"_id_"+_measureAreaId+"_"+frameId+".dat");
//          fs::path p =  polygonPath / trajFileName;
//          string polygon = p.string();
//          ofstream polys (polygon.c_str());
//
//          if(polys.is_open())
//          {
//               //for(vector<polygon_2d> polygon_iterator=polygons.begin(); polygon_iterator!=polygons.end(); polygon_iterator++)
//               for(auto && polygon_id:polygons_id)
//               {
//                    poly = polygon_id.first;
//                    for(auto&& point:poly.outer())
//                    {
//                         point.x(point.x()*CMtoM);
//                         point.y(point.y()*CMtoM);
//                    }
//                    for(auto&& innerpoly:poly.inners())
//                    {
//                         for(auto&& point:innerpoly)
//                         {
//                              point.x(point.x()*CMtoM);
//                              point.y(point.y()*CMtoM);
//                         }
//                    }
//                    polys << polygon_id.second << " | " << dsv(poly) << endl;
//                    //polys  <<dsv(poly)<< endl;
//               }
//          }
//          else
//          {
//               Log->Write("ERROR:\tcannot create the file <%s>",polygon.c_str());
//               exit(EXIT_FAILURE);
//          }
//          fs::path speedPath=voroLocPath / "speed";
//          if(!fs::exists(speedPath))
//               if(!fs::create_directory(speedPath))
//               {
//                    Log->Write("ERROR:\tcan not create directory <%s>", speedPath.string().c_str());
//                    exit(EXIT_FAILURE);
//               }
//          fs::path pv = speedPath /trajFileName;
//          string v_individual= pv.string();
//          ofstream velo (v_individual.c_str());
//          if(velo.is_open())
//          {
//               for(int pts=0; pts<numPedsInFrame; pts++)
//               {
//                    velo << fabs(VInFrame[pts]) << endl;
//               }
//          }
//          else
//          {
//               Log->Write("ERROR:\tcannot create the file <%s>",pv.string().c_str());
//               exit(EXIT_FAILURE);
//          }
//
//          *//*string point=voronoiLocation+"/points"+_trajName+"_id_"+_measureAreaId+"_"+frameId+".dat";
//            ofstream points (point.c_str());
//            if( points.is_open())
//            {
//            for(int pts=0; pts<numPedsInFrame; pts++)
//            {
//            points << XInFrame[pts]*CMtoM << "\t" << YInFrame[pts]*CMtoM << endl;
//            }
//            }
//            else
//            {
//            Log->Write("ERROR:\tcannot create the file <%s>",point.c_str());
//            exit(EXIT_FAILURE);
//            }
//
//          if(_plotVoronoiCellData)
//          {
//               string parameters_rho=" " + _scriptsLocation.string()+"/_Plot_cell_rho.py -f \""+ voroLocPath.string() + "\" -n "+ _trajName.string()+"_id_"+_measureAreaId+"_"+frameId+
//                    " -g "+_geometryFileName.string()+" -p "+_trajectoryPath.string();
//               string parameters_v=" " + _scriptsLocation.string()+"/_Plot_cell_v.py -f \""+ voroLocPath.string() + "\" -n "+ _trajName.string() + "_id_"+_measureAreaId+"_"+frameId+
//                    " -g "+_geometryFileName.string()+" -p "+_trajectoryPath.string();
//
//               if(_plotVoronoiIndex)
//                    parameters_rho += " -i";
//
//               Log->Write("INFO:\t%s",parameters_rho.c_str());
//               Log->Write("INFO:\tPlotting Voronoi Cell at the frame <%s>",frameId.c_str());
//               parameters_rho = PYTHON + parameters_rho;
//               parameters_v = PYTHON + parameters_v;
//               system(parameters_rho.c_str());
//               system(parameters_v.c_str());
//          }
//          //points.close();
//          polys.close();
//          velo.close();
//     }


// ToDo: this function GetIndividualFD is obsolete - can be deleted

//     void Method_I::GetIndividualFD(const vector<polygon_2d>& polygon, const vector<double>& Velocity, const vector<int>& Id, const string& frid)
//     {
//          double uniquedensity=0;
//          double uniquevelocity=0;
//          int uniqueId=0;
//          int temp=0;
//          for (const auto & polygon_iterator:polygon)
//          {
//               string polygon_str = polygon_to_string(polygon_iterator);
//               uniquedensity=1.0/(area(polygon_iterator)*CMtoM*CMtoM);
//               uniquevelocity=Velocity[temp];
//               uniqueId=Id[temp];
//               fprintf(_fIndividualFD,"%s\t%d\t%.3f\t%.3f\t%s\n",
//                       frid.c_str(),
//                       uniqueId,
//                       uniquedensity,
//                       uniquevelocity,
//                       polygon_str.c_str()
//                    );
//               temp++;
//          }
//     }

void Method_I::GetIndividualFD(const vector<polygon_2d>& polygon, const vector<double>& Velocity, const vector<int>& Id, const string& frid, vector<double>& XInFrame, vector<double>& YInFrame, vector<double>& ZInFrame)
{
     double uniquedensity=0;
     double uniquevelocity=0;
     double x, y, z;
     int uniqueId=0;
     int temp=0;
     for (const auto & polygon_iterator:polygon)
     {
          string polygon_str = polygon_to_string(polygon_iterator);
          uniquedensity=1.0/(area(polygon_iterator)*CMtoM*CMtoM);
          uniquevelocity=Velocity[temp];
          uniqueId=Id[temp];
          x = XInFrame[temp]*CMtoM;
          y = YInFrame[temp]*CMtoM;
          z = ZInFrame[temp]*CMtoM;
          fprintf(_fIndividualFD,"%s\t %d\t %.4f\t %.4f\t %.4f\t %.4f\t %.4f\t %s\n",
                  frid.c_str(),
                  uniqueId,
                  x,
                  y,
                  z,
                  uniquedensity,
                  uniquevelocity,
                  polygon_str.c_str()
          );
          temp++;
     }
}

     void Method_I::SetCalculateIndividualFD(bool individualFD)
     {
          _calcIndividualFD = true ;
     }

     void Method_I::SetStartFrame(int startFrame)
     {
          _startFrame=startFrame;
     }

     void Method_I::SetStopFrame(int stopFrame)
     {
          _stopFrame=stopFrame;
     }

     void Method_I::Setcutbycircle(double radius,int edges)
     {
          _cutByCircle=true;
          _cutRadius = radius;
          _circleEdges = edges;
     }

     void Method_I::SetGeometryPolygon(polygon_2d geometryPolygon)
     {
          _geoPoly = geometryPolygon;
     }

     void Method_I::SetGeometryBoundaries(double minX, double minY, double maxX, double maxY)
     {
          _geoMinX = minX;
          _geoMinY = minY;
          _geoMaxX = maxX;
          _geoMaxY = maxY;
     }

     void Method_I::SetGeometryFileName(const fs::path& geometryFile)
     {
          _geometryFileName=geometryFile;
     }

     void Method_I::SetTrajectoriesLocation(const fs::path& trajectoryPath)
     {
          _trajectoryPath=trajectoryPath;
     }

     void Method_I::SetGridSize(double x, double y)
     {
          _grid_size_X = x;
          _grid_size_Y = y;
     }


// ToDo: obsolete ?

//     void Method_I::SetCalculateProfiles(bool calcProfile)
//     {
//          _getProfile = false ;
//     }

     void Method_I::SetOutputVoronoiCellData(bool outputCellData)
     {
          _outputVoronoiCellData = outputCellData;
     }

// ToDo: obsolete ?
//     void Method_I::SetPlotVoronoiGraph(bool plotVoronoiGraph)
//     {
//          _plotVoronoiCellData = plotVoronoiGraph;
//     }

     void Method_I::SetPlotVoronoiIndex(bool plotVoronoiIndex)
     {
          _plotVoronoiIndex = plotVoronoiIndex;
     }

     void Method_I::SetMeasurementArea (MeasurementArea_B* area)
     {
          _areaForMethod_I = area;
     }

     void Method_I::SetDimensional (bool dimension)
     {
          _isOneDimensional = dimension;
     }

     void Method_I::ReducePrecision(polygon_2d& polygon)
     {
          for(auto&& point:polygon.outer())
          {
               point.x(round(point.x() * 100000000000.0) / 100000000000.0);
               point.y(round(point.y() * 100000000000.0) / 100000000000.0);
          }
     }

// ToDo: obsolete ?

//     bool Method_I::IsPedInGeometry(int frames, int peds, double **Xcor, double **Ycor, int  *firstFrame, int *lastFrame)
//     {
//          for(int i=0; i<peds; i++)
//               for(int j =0; j<frames; j++)
//               {
//                    if (j>firstFrame[i] && j< lastFrame[i] && (false==within(point_2d(round(Xcor[i][j]), round(Ycor[i][j])), _geoPoly)))
//                    {
//                         Log->Write("Error:\tPedestrian at the position <x=%.4f, y=%.4f> is outside geometry. Please check the geometry or trajectory file!", Xcor[i][j]*CMtoM, Ycor[i][j]*CMtoM );
//                         return false;
//                    }
//               }
//          return true;
//     }

     void Method_I::CalcVoronoiResults1D(vector<double>& XInFrame, vector<double>& VInFrame, vector<int>& IdInFrame, const polygon_2d & measureArea,const string& frid)
     {
          vector<double> measurearea_x;
          for(unsigned int i=0;i<measureArea.outer().size();i++)
          {
               measurearea_x.push_back(measureArea.outer()[i].x());
          }
          double left_boundary=*min_element(measurearea_x.begin(),measurearea_x.end());
          double right_boundary=*max_element(measurearea_x.begin(),measurearea_x.end());

          vector<double> voronoi_distance;
          vector<double> Xtemp=XInFrame;
          vector<double> dist;
          vector<double> XLeftNeighbor;
          vector<double> XLeftTemp;
          vector<double> XRightNeighbor;
          vector<double> XRightTemp;
          sort(Xtemp.begin(),Xtemp.end());
          dist.push_back(Xtemp[1]-Xtemp[0]);
          XLeftTemp.push_back(2*Xtemp[0]-Xtemp[1]);
          XRightTemp.push_back(Xtemp[1]);
          for(unsigned int i=1;i<Xtemp.size()-1;i++)
          {
               dist.push_back((Xtemp[i+1]-Xtemp[i-1])/2.0);
               XLeftTemp.push_back(Xtemp[i-1]);
               XRightTemp.push_back(Xtemp[i+1]);
          }
          dist.push_back(Xtemp[Xtemp.size()-1]-Xtemp[Xtemp.size()-2]);
          XLeftTemp.push_back(Xtemp[Xtemp.size()-2]);
          XRightTemp.push_back(2*Xtemp[Xtemp.size()-1]-Xtemp[Xtemp.size()-2]);
          for(unsigned int i=0;i<XInFrame.size();i++)
          {
               for(unsigned int j=0;j<Xtemp.size();j++)
               {
                    if(fabs(XInFrame[i]-Xtemp[j])<1.0e-5)
                    {
                         voronoi_distance.push_back(dist[j]);
                         XLeftNeighbor.push_back(XLeftTemp[j]);
                         XRightNeighbor.push_back(XRightTemp[j]);
                         break;
                    }
               }
          }

          double VoronoiDensity=0;
          double VoronoiVelocity=0;
          for(unsigned int i=0; i<XInFrame.size(); i++)
          {
               double ratio=getOverlapRatio((XInFrame[i]+XLeftNeighbor[i])/2.0, (XRightNeighbor[i]+XInFrame[i])/2.0,left_boundary,right_boundary);
               VoronoiDensity+=ratio;
               VoronoiVelocity+=(VInFrame[i]*voronoi_distance[i]*ratio*CMtoM);
               if(_calcIndividualFD)
               {
                    double headway=(XRightNeighbor[i] - XInFrame[i])*CMtoM;
                    double individualDensity = 2.0/((XRightNeighbor[i] - XLeftNeighbor[i])*CMtoM);
                    fprintf(_fIndividualFD,"%s\t%d\t%.3f\t%.3f\t%.3f\n",frid.c_str(), IdInFrame[i], individualDensity,VInFrame[i], headway);
               }
          }
          VoronoiDensity/=((right_boundary-left_boundary)*CMtoM);
          VoronoiVelocity/=((right_boundary-left_boundary)*CMtoM);
          fprintf(_fVoronoiRhoV,"%s\t%.3f\t%.3f\n",frid.c_str(),VoronoiDensity, VoronoiVelocity);

     }

     double Method_I::getOverlapRatio(const double& left, const double& right, const double& measurearea_left, const double& measurearea_right)
     {
          double OverlapRatio=0;
          double PersonalSpace=right-left;
          if(left > measurearea_left && right < measurearea_right) //case1
          {
               OverlapRatio=1;
          }
          else if(right > measurearea_left && right < measurearea_right && left < measurearea_left)
          {
               OverlapRatio=(right-measurearea_left)/PersonalSpace;
          }
          else if(left < measurearea_left && right > measurearea_right)
          {
               OverlapRatio=(measurearea_right - measurearea_left)/PersonalSpace;
          }
          else if(left > measurearea_left && left < measurearea_right && right > measurearea_right)
          {
               OverlapRatio=(measurearea_right-left)/PersonalSpace;
          }
          return OverlapRatio;
     }

     bool Method_I::IsPointsOnOneLine(vector<double>& XInFrame, vector<double>& YInFrame)
     {
          double deltaX=XInFrame[1] - XInFrame[0];
          bool isOnOneLine=true;
          if(fabs(deltaX)<dmin)
          {
               for(unsigned int i=2; i<XInFrame.size();i++)
               {
                    if(fabs(XInFrame[i] - XInFrame[0])> dmin)
                    {
                         isOnOneLine=false;
                         break;
                    }
               }
          }
          else
          {
               double slope=(YInFrame[1] - YInFrame[0])/deltaX;
               double intercept=YInFrame[0] - slope*XInFrame[0];
               for(unsigned int i=2; i<XInFrame.size();i++)
               {
                    double dist=fabs(slope*XInFrame[i] - YInFrame[i] + intercept)/sqrt(slope*slope +1);
                    if(dist > dmin)
                    {
                         isOnOneLine=false;
                         break;
                    }
               }
          }
          return isOnOneLine;
     }
