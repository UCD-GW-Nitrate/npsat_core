#ifndef SCATTERINTERP_H
#define SCATTERINTERP_H

#include <fstream>

#include <deal.II/base/point.h>

#include "cgal_functions.h"
#include "helper_functions.h"

/*!
 * \brief The SCI_TYPE enum can take one of the 3 values
 * - FULL in 3D this interpolates in 3D space. However this is possible only if the interpolation
 * has z information therefore it has to be STRATIFIED. In 2D problems this interpolates along the x-y plane.
 * or x-z plane if the y coordinate it is supposed to be the z. In 2D, it can be either SIMPLE or STRATIFIED
 * - HOR interpolation in 3D is a 2d interpolation across x-y. The values along Z fo not change.
 * - VERT is defined in 3D problems and it is an interpolation across an a vertical plane which is defined by 2
 * points.
 */
enum SCI_TYPE { FULL, HOR, VERT };

using namespace dealii;

//! Interpolation Class
/*!
 * The class it is written in a dimension independent style however has some limitations on 3D interpolation.
 * For the 2D case works as expected. For 3D however the function assumes that scattered points are structured
 * into layers and it does not support fully unstructured 3D interpolation. For 1D interpolation the class uses
 * a simple linear interpolation.
 *
 * The class has to main functionalities
 * i) reads scattered data from a file. ii) interpolates value of a given point.
 */
template<int dim>
class ScatterInterp{
public:
    //! The constractor just initialize default value for the number of data and the number of points.
    ScatterInterp();

    //! Reads the data from file.
    /*!
     * \brief get_data
     * \param filename is the name of the file that contains the data.
     *
     * The format of the file must be the following:
     * The first line should have the keyword SCATTERED which indicates that the data
     * that follow data correspond to scattered interpolation
     *
     * The second line is the keyward STRATIFIED or SIMPLE
     *
     * The second line provides two numbers #Npnts and #Ndata.
     *
     * The following lines after that expect an array of #Npnts rows and #Ndata+2 columns with the following format
     * X Y V_1 Z_1 V_2 Z_2 ... V_lay-1 Z_lay-1 V_lay
     *
     * The above line is repeated #Npnts times
     *
     * For example if we assume that the interpolation data are structured into 3 layers then one should provide
     * x and y coordinates, 2 pairs of values v, z, and an additional value. It the point in question has higher z
     * that the elevation of the first layer the method returns the value that corresponds to the first layer. If the
     * elevation of the point is lower than the layer above the last minus one layer then the interpolation returns the last value
     * In short the method expects for each 2d scattered point the following info
     *
     *      v1
     * ----------z1
     *      v2
     * ----------z2
     *      v3
     * ----------z3
     *      .
     *      .
     *      v(lay-1)
     * ----------z(lay-1)
     *      vlay
     */
    void get_data(std::string filename);

    /*!
     * \brief interpolate calculates the interpolation.
     * \param p The point which we want to find its value
     * \return The interpolated value
     */
    double interpolate(Point<dim> p)const;

    void set_normalized(Point<dim> a, Point<dim> b);

private:

    //! this is a container to hold the triangulation of the 2D scattered data
    ine_Delaunay_triangulation T;

    //! This is a map between the triangulation and the values that correspond to each 2D point
    std::vector<std::map<ine_Point2, ine_Coord_type, ine_Kernel::Less_xy_2> > function_values;

    //! Ndata is the number of values for interpolation. For the STRITIFIED option this number must be equal to (Nlay-1)*2 +1
    unsigned int Ndata;

    //! Npnts is the number of 2D points that the scattered interpolation set contains
    unsigned int Npnts;

    //! X_1D is the vector of x of points for the 1D interpolation of a function y=f(x)
    std::vector<double> X_1D;

    //! V_1D is the vector of vectors y of points for the 1D interpolation of list of functions of the form y_1=f(x), y_2=f(x), y_3=f(x) etc.
    //! It make sense to define more than one data for stratified hydraulic conductivity in 2D. See the discussion above.
    std::vector<std::vector<double>> V_1D;

    bool Stratified;
    bool normalized;
    SCI_TYPE sci_type;
    Point<dim> P1;
    Point<dim> P2;

};

template<int dim>
ScatterInterp<dim>::ScatterInterp(){
    Ndata = 0;
    Npnts = 0;
}

template <int dim>
void ScatterInterp<dim>::set_normalized(Point<dim> a, Point<dim> b){
    if (dim == 3 && Stratified){
        P1 = a;
        P2 = b;
        normalized = true;
    }
    else{
        std::cout << "Doesn't make sence to use normalized interpolation in other than 2D and stratified" << std::endl;
    }
}

template <int dim>
void ScatterInterp<dim>::get_data(std::string filename){
    std::ifstream  datafile(filename.c_str());
    if (!datafile.good()){
        std::cerr << "Can't open " << filename << std::endl;
        return;
    }
    else{
        char buffer[512];
        {// Read the data type
            datafile.getline(buffer, 512);
            std::istringstream inp(buffer);
            std::string temp;
            inp >> temp;
            if (temp != "SCATTERED"){
                std::cerr << " ScatterInterp Cannot read " << temp << " data." << std::endl;
                return;
            }
        }

        {// Read SCI_TYPE
            datafile.getline(buffer, 512);
            std::istringstream inp(buffer);
            std::string temp;
            inp >> temp;
            if (temp == "FULL")
                sci_type = FULL;
            else if (temp == "HOR")
                sci_type = HOR;
            else if (temp == "VERT")
                sci_type = VERT;
            else
                std::cout << "Unkown interpolation type. Valid options are FULL, HOR, VERT" << std::endl;
        }

        {// Read interpolation style
            datafile.getline(buffer, 512);
            std::istringstream inp(buffer);
            std::string temp;
            inp >> temp;
            if (temp == "STRATIFIED")
                Stratified = true;
            else if (temp == "SIMPLE")
                Stratified = false;
            else
                std::cout << "Unknown interpolation style. Valid options are STRATIFIED or SIMPLE" << std::endl;

        }

        {//Read number of points and number of data
            datafile.getline(buffer,512);
            std::istringstream inp(buffer);
            inp >> Npnts;
            inp >> Ndata;
            function_values.resize(Ndata);
        }
        {// Read the actual data
            double x, y, v;
            for (unsigned int i = 0; i < Npnts; ++i){
                datafile.getline(buffer, 512);
                std::istringstream inp(buffer);
                if (dim == 1 || (dim == 2 && Stratified) || normalized){
                    inp >> x;
                    X_1D.push_back(x);
                    std::vector<double> temp;
                    for (unsigned int j = 0; j < Ndata; ++j){
                        inp >> v;
                        temp.push_back(v);
                    }
                    // Here we need to store the values
                    V_1D.push_back(temp);
                }
                else{
                    inp >> x;
                    inp >> y;
                    ine_Point2 p(x, y);
                    T.insert(p);
                    for (unsigned int j = 0; j < Ndata; ++j){
                        inp >> v;
                        ine_Coord_type ct(v);
                        function_values[j].insert(std::make_pair(p,ct));
                    }
                }
            }
        }// Read data block

    }
}

template <int dim>
double ScatterInterp<dim>::interpolate(Point<dim> point)const{    
    if (dim == 3){
        if (sci_type == FULL){
            Point<3> pp;
            pp[0] = point[0];
            pp[1] = point[1];
            pp[2] = point[2];
            return scatter_2D_interpolation(T, function_values, point);
        }
        else if (sci_type == HOR){
            Point<3> pp;
            pp[0] = point[0];
            pp[1] = point[1];
            pp[2] = 0;
            return scatter_2D_interpolation(T, function_values, point);
        }
        else if (sci_type == VERT){
            if (!Stratified){
                std::cerr << "Not implemented yet, but its easy to do so" << std::endl;
                return 0;
            }
            else{
                // find the distance from the first point
                double xx = distance_on_2D_line(P1[0], P1[1], P2[0], P2[1], point[0], point[1]);
            }
        }
    }
    else if (dim == 2){

    }
    Point<3> pp;
    if (dim == 1 || (dim == 2 && Stratified) || normalized){
        // find main parametric value
        double t;
        int ind;
        bool first = false;
        bool last = false;
        double xx = 0;
        double zz = 0;
        double yy = 0;
        if (dim == 1)
            xx = point[0];
        else if (dim == 2 && Stratified){
            xx = point[0];
            yy = point[dim-1];
        }
        else if (normalized){

        }

        if (xx <=X_1D[0])
            first = true;
        else if (xx >= X_1D[X_1D.size()-1])
            last = true;
        else{
            for (unsigned int i = 0; i < X_1D.size()-1; ++i){
                if (xx >= X_1D[i] && xx <= X_1D[i+1]){
                    t = (xx - X_1D[i]) / (X_1D[i+1] - X_1D[i]);
                    ind = i;
                    break;
                }
            }
        }

        if (dim == 1){
            if (first)
                return V_1D[0][0];
            else if (last)
                return  V_1D[V_1D.size()-1][0];
            else{
                return V_1D[ind][0]*(1-t) + V_1D[ind+1][0]*t;
            }
        }
        else if (dim == 2 && Stratified){
            std::vector<double> v;
            std::vector<double> el;
            bool push_v = true;
            double v_temp;
            for (unsigned int i = 0; i < Ndata; ++i){
                if (first)
                    v_temp = V_1D[0][i];
                else if (last)
                    v_temp = V_1D[V_1D.size()-1][i];
                else{
                    v_temp = V_1D[ind][i]*(1-t) + V_1D[ind+1][i]*t;
                }

                if (push_v){
                    v.push_back(v_temp);
                    push_v = false;
                }
                else{
                    el.push_back(v_temp);
                    push_v = true;
                }
            }

            if (point[1] <= el[0])
                return v[0];
            else if (point[1]>=el[el.size()-1])
                return v[v.size()-1];
            else{
                for (unsigned int i = 0; i < el.size()-1; ++i){
                    if (point[1] >= el[i] && point[1] <= el[i+1]){
                        double u = (point[1] - el[i])/(el[i+1] - el[i]);
                        return v[i]*(1-u) + v[i+1]*u;
                    }
                }
            }

        }
    }
    else if (dim == 2) {
        pp[0] = point[0];
        pp[1] = point[1];
        pp[2] = 0;
    } else if (dim == 3){
        pp[0] = point[0];
        pp[1] = point[1];
        pp[2] = point[2];
    }

    double value = scatter_2D_interpolation(T, function_values, pp);
    return  value;
}


#endif // SCATTERINTERP_H
