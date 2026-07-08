#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/SpatialTransform.h>
#include <OpenSim/Simulation/SimbodyEngine/CustomJoint.h>
#include <OpenSim/Common/LinearFunction.h>
#include <OpenSim/Common/PolynomialFunction.h>
#include <OpenSim/Common/MultiplierFunction.h>
#include <OpenSim/Common/Constant.h>
#include <OpenSim/Simulation/Model/SmoothSphereHalfSpaceForce.h>
#include "SimTKcommon/internal/recorder.h"

#include <iostream>
#include <iterator>
#include <random>
#include <cassert>
#include <algorithm>
#include <vector>
#include <fstream>

using namespace SimTK;
using namespace OpenSim;

constexpr int n_in = 2; 
constexpr int n_out = 1; 
constexpr int nCoordinates = 33; 
constexpr int NX = nCoordinates*2; 
constexpr int NU = nCoordinates; 
constexpr int NR = nCoordinates + 3*4 + 3*2*12; 

template<typename T> 
T value(const Recorder& e) { return e; }; 
template<> 
double value(const Recorder& e) { return e.getValue(); }; 

SimTK::Array_<int> getIndicesOSInSimbody(const Model& model) { 
	auto s = model.getWorkingState(); 
	const auto svNames = model.getStateVariableNames(); 
	SimTK::Array_<int> idxOSInSimbody(s.getNQ()); 
	s.updQ() = 0; 
	for (int iy = 0; iy < s.getNQ(); ++iy) { 
		s.updQ()[iy] = SimTK::NaN; 
		const auto svValues = model.getStateVariableValues(s); 
		for (int isv = 0; isv < svNames.size(); ++isv) { 
			if (SimTK::isNaN(svValues[isv])) { 
				s.updQ()[iy] = 0; 
				idxOSInSimbody[iy] = isv/2; 
				break; 
			} 
		} 
	} 
	return idxOSInSimbody; 
} 

SimTK::Array_<int> getIndicesSimbodyInOS(const Model& model) { 
	auto idxOSInSimbody = getIndicesOSInSimbody(model); 
	auto s = model.getWorkingState(); 
	SimTK::Array_<int> idxSimbodyInOS(s.getNQ()); 
	for (int iy = 0; iy < s.getNQ(); ++iy) { 
		for (int iyy = 0; iyy < s.getNQ(); ++iyy) { 
			if (idxOSInSimbody[iyy] == iy) { 
				idxSimbodyInOS[iy] = iyy; 
				break; 
			} 
		} 
	} 
	return idxSimbodyInOS; 
} 

template<typename T>
int F_generic(const T** arg, T** res) {
	OpenSim::Model* model;
	model = new OpenSim::Model();;

	// Definition of bodies
	OpenSim::Body* pelvis;
	pelvis = new OpenSim::Body("pelvis", 11.25534597873554965020, Vec3(-0.07487474805565795533, 0.00000000000000000000, 0.00000000000000000000), Inertia(0.09824654552212061520, 0.08324196609899518762, 0.05533535978337338068, 0., 0., 0.));
	model->addBody(pelvis);

	OpenSim::Body* femur_r;
	femur_r = new OpenSim::Body("femur_r", 8.88940095836043475686, Vec3(0.00000000000000000000, -0.17483393216717885155, 0.00000000000000000000), Inertia(0.13535003089100633433, 0.03548010518502107968, 0.14272908410612467067, 0., 0., 0.));
	model->addBody(femur_r);

	OpenSim::Body* tibia_r;
	tibia_r = new OpenSim::Body("tibia_r", 3.54327886695780325255, Vec3(0.00000000000000000000, -0.19840967328771910716, 0.00000000000000000000), Inertia(0.05439910553293379053, 0.00550467139321353903, 0.05515464866533564825, 0., 0., 0.));
	model->addBody(tibia_r);

	OpenSim::Body* talus_r;
	talus_r = new OpenSim::Body("talus_r", 0.09557056957404730380, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Inertia(0.00089628755527230487, 0.00089628755527230487, 0.00089628755527230487, 0., 0., 0.));
	model->addBody(talus_r);

	OpenSim::Body* calcn_r;
	calcn_r = new OpenSim::Body("calcn_r", 1.19463211967559113091, Vec3(0.09684151969953397332, 0.02905245590986018853, 0.00000000000000000000), Inertia(0.00125480257738122677, 0.00349552146556198840, 0.00367477897661645011, 0., 0., 0.));
	model->addBody(calcn_r);

	OpenSim::Body* toes_r;
	toes_r = new OpenSim::Body("toes_r", 0.20700585369738641606, Vec3(0.03350716581603874894, 0.00581049118197203823, -0.01694726594741844533), Inertia(0.00008962875552723049, 0.00017925751105446098, 0.00089628755527230487, 0., 0., 0.));
	model->addBody(toes_r);

	OpenSim::Body* femur_l;
	femur_l = new OpenSim::Body("femur_l", 8.88940095836043475686, Vec3(0.00000000000000000000, -0.17602855717485405007, 0.00000000000000000000), Inertia(0.13720602011647481389, 0.03596662663247397751, 0.14468625870385543042, 0., 0., 0.));
	model->addBody(femur_l);

	OpenSim::Body* tibia_l;
	tibia_l = new OpenSim::Body("tibia_l", 3.54327886695780325255, Vec3(0.00000000000000000000, -0.19929124620647029054, 0.00000000000000000000), Inertia(0.05488359117428829453, 0.00555369672596964931, 0.05564586327393119397, 0., 0., 0.));
	model->addBody(tibia_l);

	OpenSim::Body* talus_l;
	talus_l = new OpenSim::Body("talus_l", 0.09557056957404730380, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Inertia(0.00089628755527230487, 0.00089628755527230487, 0.00089628755527230487, 0., 0., 0.));
	model->addBody(talus_l);

	OpenSim::Body* calcn_l;
	calcn_l = new OpenSim::Body("calcn_l", 1.19463211967559113091, Vec3(0.09684151969953397332, 0.02905245590986018853, 0.00000000000000000000), Inertia(0.00125480257738122677, 0.00349552146556198840, 0.00367477897661645011, 0., 0., 0.));
	model->addBody(calcn_l);

	OpenSim::Body* toes_l;
	toes_l = new OpenSim::Body("toes_l", 0.20700585369738641606, Vec3(0.03350716581603874894, 0.00581049118197203823, 0.01694726594741844533), Inertia(0.00008962875552723049, 0.00017925751105446098, 0.00089628755527230487, 0., 0., 0.));
	model->addBody(toes_l);

	OpenSim::Body* torso;
	torso = new OpenSim::Body("torso", 25.63833441735136986495, Vec3(-0.03227947325045067706, 0.34431438133814057379, 0.00000000000000000000), Inertia(1.40918804836932731739, 0.72203565313192730191, 1.36799713288291302504, 0., 0., 0.));
	model->addBody(torso);

	OpenSim::Body* humerus_r;
	humerus_r = new OpenSim::Body("humerus_r", 1.94247182659251138581, Vec3(0.00000000000000000000, -0.17041342030664896612, 0.00000000000000000000), Inertia(0.01225213874146988818, 0.00422660838386048916, 0.01375263087095008642, 0., 0., 0.));
	model->addBody(humerus_r);

	OpenSim::Body* ulna_r;
	ulna_r = new OpenSim::Body("ulna_r", 0.58059121016233738999, Vec3(0.00000000000000000000, -0.12753496551167098350, 0.00000000000000000000), Inertia(0.00316966589016490956, 0.00066132799463940390, 0.00343826350611068764, 0., 0., 0.));
	model->addBody(ulna_r);

	OpenSim::Body* radius_r;
	radius_r = new OpenSim::Body("radius_r", 0.58059121016233738999, Vec3(0.00000000000000000000, -0.12753496551167098350, 0.00000000000000000000), Inertia(0.00316966589016490956, 0.00066132799463940390, 0.00343826350611068764, 0., 0., 0.));
	model->addBody(radius_r);

	OpenSim::Body* hand_r;
	hand_r = new OpenSim::Body("hand_r", 0.43723535580126637878, Vec3(0.00000000000000000000, -0.07205553600097272526, 0.00000000000000000000), Inertia(0.00095453814113001338, 0.00058535018295753053, 0.00143394743174239691, 0., 0., 0.));
	model->addBody(hand_r);

	OpenSim::Body* humerus_l;
	humerus_l = new OpenSim::Body("humerus_l", 1.94247182659251138581, Vec3(0.00000000000000000000, -0.17041342030664896612, 0.00000000000000000000), Inertia(0.01225213874146988818, 0.00422660838386048916, 0.01375263087095008642, 0., 0., 0.));
	model->addBody(humerus_l);

	OpenSim::Body* ulna_l;
	ulna_l = new OpenSim::Body("ulna_l", 0.58059121016233738999, Vec3(0.00000000000000000000, -0.12753496551167098350, 0.00000000000000000000), Inertia(0.00316966589016490956, 0.00066132799463940390, 0.00343826350611068764, 0., 0., 0.));
	model->addBody(ulna_l);

	OpenSim::Body* radius_l;
	radius_l = new OpenSim::Body("radius_l", 0.58059121016233738999, Vec3(0.00000000000000000000, -0.12753496551167098350, 0.00000000000000000000), Inertia(0.00316966589016490956, 0.00066132799463940390, 0.00343826350611068764, 0., 0., 0.));
	model->addBody(radius_l);

	OpenSim::Body* hand_l;
	hand_l = new OpenSim::Body("hand_l", 0.43723535580126637878, Vec3(0.00000000000000000000, -0.07205553600097272526, 0.00000000000000000000), Inertia(0.00095453814113001338, 0.00058535018295753053, 0.00143394743174239691, 0., 0., 0.));
	model->addBody(hand_l);

	// Definition of joints
	SpatialTransform st_ground_pelvis;
	st_ground_pelvis[0].setCoordinateNames(OpenSim::Array<std::string>("pelvis_tilt", 1, 1));
	st_ground_pelvis[0].setFunction(new LinearFunction(1.0000, 0.0000));
	st_ground_pelvis[0].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	st_ground_pelvis[1].setCoordinateNames(OpenSim::Array<std::string>("pelvis_list", 1, 1));
	st_ground_pelvis[1].setFunction(new LinearFunction(1.0000, 0.0000));
	st_ground_pelvis[1].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_ground_pelvis[2].setCoordinateNames(OpenSim::Array<std::string>("pelvis_rotation", 1, 1));
	st_ground_pelvis[2].setFunction(new LinearFunction(1.0000, 0.0000));
	st_ground_pelvis[2].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_ground_pelvis[3].setCoordinateNames(OpenSim::Array<std::string>("pelvis_tx", 1, 1));
	st_ground_pelvis[3].setFunction(new LinearFunction(1.0000, 0.0000));
	st_ground_pelvis[3].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_ground_pelvis[4].setCoordinateNames(OpenSim::Array<std::string>("pelvis_ty", 1, 1));
	st_ground_pelvis[4].setFunction(new LinearFunction(1.0000, 0.0000));
	st_ground_pelvis[4].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_ground_pelvis[5].setCoordinateNames(OpenSim::Array<std::string>("pelvis_tz", 1, 1));
	st_ground_pelvis[5].setFunction(new LinearFunction(1.0000, 0.0000));
	st_ground_pelvis[5].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	OpenSim::CustomJoint* ground_pelvis;
	ground_pelvis = new OpenSim::CustomJoint("ground_pelvis", model->getGround(), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), *pelvis, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), st_ground_pelvis);

	SpatialTransform st_hip_r;
	st_hip_r[0].setCoordinateNames(OpenSim::Array<std::string>("hip_flexion_r", 1, 1));
	st_hip_r[0].setFunction(new LinearFunction(1.0000, 0.0000));
	st_hip_r[0].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	st_hip_r[1].setCoordinateNames(OpenSim::Array<std::string>("hip_adduction_r", 1, 1));
	st_hip_r[1].setFunction(new LinearFunction(1.0000, 0.0000));
	st_hip_r[1].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_hip_r[2].setCoordinateNames(OpenSim::Array<std::string>("hip_rotation_r", 1, 1));
	st_hip_r[2].setFunction(new LinearFunction(1.0000, 0.0000));
	st_hip_r[2].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_hip_r[3].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.05904877023561461158));
	st_hip_r[3].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_hip_r[4].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.05904877023561461158));
	st_hip_r[4].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_hip_r[5].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.18482290504437615120));
	st_hip_r[5].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	OpenSim::CustomJoint* hip_r;
	hip_r = new OpenSim::CustomJoint("hip_r", *pelvis, Vec3(-0.05959902859377944756, -0.08312473797579339463, 0.09153941764372849899), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), *femur_r, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), st_hip_r);

	SpatialTransform st_walker_knee_r;
	st_walker_knee_r[0].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_r", 1, 1));
	st_walker_knee_r[0].setFunction(new LinearFunction(1.0000, 0.0000));
	st_walker_knee_r[0].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_walker_knee_r[1].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_r", 1, 1));
	osim_double_adouble st_walker_knee_r_1_coeffs[5] = {0.01083209453986300023, -0.02521832550124099986, -0.03284781039885199816, 0.07910001196702699799, -0.00000001473252350900}; 
	Vector st_walker_knee_r_1_coeffs_vec(5); 
	for (int i = 0; i < 5; ++i) st_walker_knee_r_1_coeffs_vec[i] = st_walker_knee_r_1_coeffs[i]; 
	st_walker_knee_r[1].setFunction(new PolynomialFunction(st_walker_knee_r_1_coeffs_vec));
	st_walker_knee_r[1].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	st_walker_knee_r[2].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_r", 1, 1));
	osim_double_adouble st_walker_knee_r_2_coeffs[4] = {0.02516576272742300155, -0.16948005139054000967, 0.36949934868824901857, -0.00000004430358308836}; 
	Vector st_walker_knee_r_2_coeffs_vec(4); 
	for (int i = 0; i < 4; ++i) st_walker_knee_r_2_coeffs_vec[i] = st_walker_knee_r_2_coeffs[i]; 
	st_walker_knee_r[2].setFunction(new PolynomialFunction(st_walker_knee_r_2_coeffs_vec));
	st_walker_knee_r[2].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_walker_knee_r[3].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_r", 1, 1));
	osim_double_adouble st_walker_knee_r_3_coeffs[5] = {0.00015904478788503811, -0.00101514991566899995, 0.00181751097496800004, 0.00002641426645199230, -0.00000077465635324719}; 
	Vector st_walker_knee_r_3_coeffs_vec(5); 
	for (int i = 0; i < 5; ++i) st_walker_knee_r_3_coeffs_vec[i] = st_walker_knee_r_3_coeffs[i]; 
	st_walker_knee_r[3].setFunction(new MultiplierFunction(new PolynomialFunction(st_walker_knee_r_3_coeffs_vec), 1.02843489510105201568));
	st_walker_knee_r[3].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_walker_knee_r[4].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_r", 1, 1));
	osim_double_adouble st_walker_knee_r_4_coeffs[5] = {-0.00057968780523386836, 0.00507976574562600015, -0.01144237572636399962, 0.00393690866884400040, -0.00002516350383213525}; 
	Vector st_walker_knee_r_4_coeffs_vec(5); 
	for (int i = 0; i < 5; ++i) st_walker_knee_r_4_coeffs_vec[i] = st_walker_knee_r_4_coeffs[i]; 
	st_walker_knee_r[4].setFunction(new MultiplierFunction(new PolynomialFunction(st_walker_knee_r_4_coeffs_vec), 1.02843489510105201568));
	st_walker_knee_r[4].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_walker_knee_r[5].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_r", 1, 1));
	osim_double_adouble st_walker_knee_r_5_coeffs[5] = {0.00120808688920599999, -0.00445361122470600002, 0.00061164940729817395, 0.00626542960638699995, -0.00001461912533723326}; 
	Vector st_walker_knee_r_5_coeffs_vec(5); 
	for (int i = 0; i < 5; ++i) st_walker_knee_r_5_coeffs_vec[i] = st_walker_knee_r_5_coeffs[i]; 
	st_walker_knee_r[5].setFunction(new MultiplierFunction(new PolynomialFunction(st_walker_knee_r_5_coeffs_vec), 1.02843489510105201568));
	st_walker_knee_r[5].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	OpenSim::CustomJoint* walker_knee_r;
	walker_knee_r = new OpenSim::CustomJoint("walker_knee_r", *femur_r, Vec3(-0.00462795702795473340, -0.42124693303339094674, -0.00179976106642684101), Vec3(-1.64156999999999997364, 1.44618000000000002103, 1.57079999999999997407), *tibia_r, Vec3(-0.00859690941857490717, -0.00375649980255720120, -0.00157786169425392655), Vec3(-1.64156999999999997364, 1.44618000000000002103, 1.57079999999999997407), st_walker_knee_r);

	OpenSim::PinJoint* ankle_r;
	ankle_r = new OpenSim::PinJoint("ankle_r", *tibia_r, Vec3(-0.01062719192756931587, -0.42508767710277262086, 0.00000000000000000000), Vec3(0.17589499999999999580, -0.10520799999999999597, 0.01866220000000000032), *talus_r, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(0.17589499999999999580, -0.10520799999999999597, 0.01866220000000000032));

	OpenSim::PinJoint* subtalar_r;
	subtalar_r = new OpenSim::PinJoint("subtalar_r", *talus_r, Vec3(-0.04722960915746271526, -0.04062501751395449640, 0.00766984836020308952), Vec3(-1.76818999999999992845, 0.90622300000000000075, 1.81960000000000010623), *calcn_r, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(-1.76818999999999992845, 0.90622300000000000075, 1.81960000000000010623));

	OpenSim::PinJoint* mtp_r;
	mtp_r = new OpenSim::PinJoint("mtp_r", *calcn_r, Vec3(0.17315263722276671987, -0.00193683039399067934, 0.00104588841275496673), Vec3(-3.14158999999999988262, 0.61990100000000003533, 0.00000000000000000000), *toes_r, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(-3.14158999999999988262, 0.61990100000000003533, 0.00000000000000000000));

	SpatialTransform st_hip_l;
	st_hip_l[0].setCoordinateNames(OpenSim::Array<std::string>("hip_flexion_l", 1, 1));
	st_hip_l[0].setFunction(new LinearFunction(1.0000, 0.0000));
	st_hip_l[0].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	st_hip_l[1].setCoordinateNames(OpenSim::Array<std::string>("hip_adduction_l", 1, 1));
	st_hip_l[1].setFunction(new LinearFunction(-1.0000, 0.0000));
	st_hip_l[1].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_hip_l[2].setCoordinateNames(OpenSim::Array<std::string>("hip_rotation_l", 1, 1));
	st_hip_l[2].setFunction(new LinearFunction(-1.0000, 0.0000));
	st_hip_l[2].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_hip_l[3].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.05904877023561461158));
	st_hip_l[3].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_hip_l[4].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.05904877023561461158));
	st_hip_l[4].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_hip_l[5].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.18482290504437615120));
	st_hip_l[5].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	OpenSim::CustomJoint* hip_l;
	hip_l = new OpenSim::CustomJoint("hip_l", *pelvis, Vec3(-0.05959902859377944756, -0.08312473797579339463, -0.09153941764372849899), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), *femur_l, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), st_hip_l);

	SpatialTransform st_walker_knee_l;
	st_walker_knee_l[0].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_l", 1, 1));
	st_walker_knee_l[0].setFunction(new LinearFunction(1.0000, 0.0000));
	st_walker_knee_l[0].setAxis(Vec3(-1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_walker_knee_l[1].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_l", 1, 1));
	osim_double_adouble st_walker_knee_l_1_coeffs[5] = {0.01083209453986300023, -0.02521832550124099986, -0.03284781039885199816, 0.07910001196702699799, -0.00000001473252350900}; 
	Vector st_walker_knee_l_1_coeffs_vec(5); 
	for (int i = 0; i < 5; ++i) st_walker_knee_l_1_coeffs_vec[i] = st_walker_knee_l_1_coeffs[i]; 
	st_walker_knee_l[1].setFunction(new PolynomialFunction(st_walker_knee_l_1_coeffs_vec));
	st_walker_knee_l[1].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	st_walker_knee_l[2].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_l", 1, 1));
	osim_double_adouble st_walker_knee_l_2_coeffs[4] = {-0.02516576272742300155, 0.16948005139054000967, -0.36949934868824901857, 0.00000004430358308836}; 
	Vector st_walker_knee_l_2_coeffs_vec(4); 
	for (int i = 0; i < 4; ++i) st_walker_knee_l_2_coeffs_vec[i] = st_walker_knee_l_2_coeffs[i]; 
	st_walker_knee_l[2].setFunction(new PolynomialFunction(st_walker_knee_l_2_coeffs_vec));
	st_walker_knee_l[2].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_walker_knee_l[3].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_l", 1, 1));
	osim_double_adouble st_walker_knee_l_3_coeffs[5] = {0.00015904478788503811, -0.00101514991566899995, 0.00181751097496800004, 0.00002641426645199230, -0.00000077465635324719}; 
	Vector st_walker_knee_l_3_coeffs_vec(5); 
	for (int i = 0; i < 5; ++i) st_walker_knee_l_3_coeffs_vec[i] = st_walker_knee_l_3_coeffs[i]; 
	st_walker_knee_l[3].setFunction(new MultiplierFunction(new PolynomialFunction(st_walker_knee_l_3_coeffs_vec), 1.03546210102855318347));
	st_walker_knee_l[3].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_walker_knee_l[4].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_l", 1, 1));
	osim_double_adouble st_walker_knee_l_4_coeffs[5] = {-0.00057968780523386836, 0.00507976574562600015, -0.01144237572636399962, 0.00393690866884400040, -0.00002516350383213525}; 
	Vector st_walker_knee_l_4_coeffs_vec(5); 
	for (int i = 0; i < 5; ++i) st_walker_knee_l_4_coeffs_vec[i] = st_walker_knee_l_4_coeffs[i]; 
	st_walker_knee_l[4].setFunction(new MultiplierFunction(new PolynomialFunction(st_walker_knee_l_4_coeffs_vec), 1.03546210102855318347));
	st_walker_knee_l[4].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_walker_knee_l[5].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_l", 1, 1));
	osim_double_adouble st_walker_knee_l_5_coeffs[5] = {-0.00120808688920599999, 0.00445361122470600002, -0.00061164940729817395, -0.00626542960638699995, 0.00001461912533723326}; 
	Vector st_walker_knee_l_5_coeffs_vec(5); 
	for (int i = 0; i < 5; ++i) st_walker_knee_l_5_coeffs_vec[i] = st_walker_knee_l_5_coeffs[i]; 
	st_walker_knee_l[5].setFunction(new MultiplierFunction(new PolynomialFunction(st_walker_knee_l_5_coeffs_vec), 1.03546210102855318347));
	st_walker_knee_l[5].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	OpenSim::CustomJoint* walker_knee_l;
	walker_knee_l = new OpenSim::CustomJoint("walker_knee_l", *femur_l, Vec3(-0.00465957945462848900, -0.42412527658129539621, 0.00181205867679996814), Vec3(1.64156999999999997364, -1.44618000000000002103, 1.57079999999999997407), *tibia_l, Vec3(-0.00863510716570481743, -0.00377319066465255050, 0.00158487244184571342), Vec3(1.64156999999999997364, -1.44618000000000002103, 1.57079999999999997407), st_walker_knee_l);

	OpenSim::PinJoint* ankle_l;
	ankle_l = new OpenSim::PinJoint("ankle_l", *tibia_l, Vec3(-0.01067441061630799645, -0.42697642465231988584, 0.00000000000000000000), Vec3(-0.17589499999999999580, 0.10520799999999999597, 0.01866220000000000032), *talus_l, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(-0.17589499999999999580, 0.10520799999999999597, 0.01866220000000000032));

	OpenSim::PinJoint* subtalar_l;
	subtalar_l = new OpenSim::PinJoint("subtalar_l", *talus_l, Vec3(-0.04722960915746271526, -0.04062501751395449640, -0.00766984836020308952), Vec3(1.76818999999999992845, -0.90622300000000000075, 1.81960000000000010623), *calcn_l, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(1.76818999999999992845, -0.90622300000000000075, 1.81960000000000010623));

	OpenSim::PinJoint* mtp_l;
	mtp_l = new OpenSim::PinJoint("mtp_l", *calcn_l, Vec3(0.17315263722276671987, -0.00193683039399067934, -0.00104588841275496673), Vec3(-3.14158999999999988262, -0.61990100000000003533, 0.00000000000000000000), *toes_l, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(-3.14158999999999988262, -0.61990100000000003533, 0.00000000000000000000));

	SpatialTransform st_back;
	st_back[0].setCoordinateNames(OpenSim::Array<std::string>("lumbar_extension", 1, 1));
	st_back[0].setFunction(new LinearFunction(1.0000, 0.0000));
	st_back[0].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	st_back[1].setCoordinateNames(OpenSim::Array<std::string>("lumbar_bending", 1, 1));
	st_back[1].setFunction(new LinearFunction(1.0000, 0.0000));
	st_back[1].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_back[2].setCoordinateNames(OpenSim::Array<std::string>("lumbar_rotation", 1, 1));
	st_back[2].setFunction(new LinearFunction(1.0000, 0.0000));
	st_back[2].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_back[3].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.05904877023561461158));
	st_back[3].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_back[4].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.05904877023561461158));
	st_back[4].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_back[5].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.18482290504437615120));
	st_back[5].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	OpenSim::CustomJoint* back;
	back = new OpenSim::CustomJoint("back", *pelvis, Vec3(-0.10664621116272639256, 0.08631247477420259107, 0.00000000000000000000), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), *torso, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), st_back);

	SpatialTransform st_acromial_r;
	st_acromial_r[0].setCoordinateNames(OpenSim::Array<std::string>("arm_flex_r", 1, 1));
	st_acromial_r[0].setFunction(new LinearFunction(1.0000, 0.0000));
	st_acromial_r[0].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	st_acromial_r[1].setCoordinateNames(OpenSim::Array<std::string>("arm_add_r", 1, 1));
	st_acromial_r[1].setFunction(new LinearFunction(1.0000, 0.0000));
	st_acromial_r[1].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_acromial_r[2].setCoordinateNames(OpenSim::Array<std::string>("arm_rot_r", 1, 1));
	st_acromial_r[2].setFunction(new LinearFunction(1.0000, 0.0000));
	st_acromial_r[2].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_acromial_r[3].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.07598244168168921675));
	st_acromial_r[3].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_acromial_r[4].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.07598244168168921675));
	st_acromial_r[4].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_acromial_r[5].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.02509502245620587324));
	st_acromial_r[5].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	OpenSim::CustomJoint* acromial_r;
	acromial_r = new OpenSim::CustomJoint("acromial_r", *torso, Vec3(0.00339472460350572934, 0.39972747708474754003, 0.17426615381755500289), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), *humerus_r, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), st_acromial_r);

	OpenSim::PinJoint* elbow_r;
	elbow_r = new OpenSim::PinJoint("elbow_r", *humerus_r, Vec3(0.01361633291091046867, -0.29656029149460383376, -0.00993979871273477887), Vec3(-0.02286269999999999969, 0.22801799999999999846, 0.00516889999999999971), *ulna_r, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(-0.02286269999999999969, 0.22801799999999999846, 0.00516889999999999971));

	OpenSim::PinJoint* radioulnar_r;
	radioulnar_r = new OpenSim::PinJoint("radioulnar_r", *ulna_r, Vec3(-0.00711825524162630879, -0.01376351210462812419, 0.02760003738179559948), Vec3(-1.56884000000000001229, 0.05642799999999999899, 1.53614000000000006096), *radius_r, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(-1.56884000000000001229, 0.05642799999999999899, 1.53614000000000006096));

	OpenSim::WeldJoint* radius_hand_r;
	radius_hand_r = new OpenSim::WeldJoint("radius_hand_r", *radius_r, Vec3(-0.00930865041780684285, -0.24955796557758141985, 0.01440158374290680283), Vec3(-1.57079999999999997407, 0.00000000000000000000, -1.57079999999999997407), *hand_r, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(-1.57079999999999997407, 0.00000000000000000000, -1.57079999999999997407));

	SpatialTransform st_acromial_l;
	st_acromial_l[0].setCoordinateNames(OpenSim::Array<std::string>("arm_flex_l", 1, 1));
	st_acromial_l[0].setFunction(new LinearFunction(1.0000, 0.0000));
	st_acromial_l[0].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	st_acromial_l[1].setCoordinateNames(OpenSim::Array<std::string>("arm_add_l", 1, 1));
	st_acromial_l[1].setFunction(new LinearFunction(1.0000, 0.0000));
	st_acromial_l[1].setAxis(Vec3(-1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_acromial_l[2].setCoordinateNames(OpenSim::Array<std::string>("arm_rot_l", 1, 1));
	st_acromial_l[2].setFunction(new LinearFunction(1.0000, 0.0000));
	st_acromial_l[2].setAxis(Vec3(0.00000000000000000000, -1.00000000000000000000, 0.00000000000000000000));
	st_acromial_l[3].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.07598244168168921675));
	st_acromial_l[3].setAxis(Vec3(1.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	st_acromial_l[4].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.07598244168168921675));
	st_acromial_l[4].setAxis(Vec3(0.00000000000000000000, 1.00000000000000000000, 0.00000000000000000000));
	st_acromial_l[5].setFunction(new MultiplierFunction(new Constant(0.00000000000000000000), 1.02509502245620587324));
	st_acromial_l[5].setAxis(Vec3(0.00000000000000000000, 0.00000000000000000000, 1.00000000000000000000));
	OpenSim::CustomJoint* acromial_l;
	acromial_l = new OpenSim::CustomJoint("acromial_l", *torso, Vec3(0.00339472460350572934, 0.39972747708474754003, -0.17426615381755500289), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), *humerus_l, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), st_acromial_l);

	OpenSim::PinJoint* elbow_l;
	elbow_l = new OpenSim::PinJoint("elbow_l", *humerus_l, Vec3(0.01361633291091046867, -0.29656029149460383376, 0.00993979871273477887), Vec3(0.02286269999999999969, -0.22801799999999999846, 0.00516889999999999971), *ulna_l, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(0.02286269999999999969, -0.22801799999999999846, 0.00516889999999999971));

	OpenSim::PinJoint* radioulnar_l;
	radioulnar_l = new OpenSim::PinJoint("radioulnar_l", *ulna_l, Vec3(-0.00711825524162630879, -0.01376351210462812419, -0.02760003738179559948), Vec3(1.56884000000000001229, -0.05642799999999999899, 1.53614000000000006096), *radius_l, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(1.56884000000000001229, -0.05642799999999999899, 1.53614000000000006096));

	OpenSim::WeldJoint* radius_hand_l;
	radius_hand_l = new OpenSim::WeldJoint("radius_hand_l", *radius_l, Vec3(-0.00930865041780684285, -0.24955796557758141985, -0.01440158374290680283), Vec3(1.57079999999999997407, 0.00000000000000000000, 1.57079999999999997407), *hand_l, Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000), Vec3(1.57079999999999997407, 0.00000000000000000000, 1.57079999999999997407));

	model->addJoint(ground_pelvis);
	model->addJoint(hip_l);
	model->addJoint(hip_r);
	model->addJoint(walker_knee_l);
	model->addJoint(walker_knee_r);
	model->addJoint(ankle_l);
	model->addJoint(ankle_r);
	model->addJoint(subtalar_l);
	model->addJoint(subtalar_r);
	model->addJoint(mtp_l);
	model->addJoint(mtp_r);
	model->addJoint(back);
	model->addJoint(acromial_l);
	model->addJoint(acromial_r);
	model->addJoint(elbow_l);
	model->addJoint(elbow_r);
	model->addJoint(radioulnar_l);
	model->addJoint(radioulnar_r);
	model->addJoint(radius_hand_l);
	model->addJoint(radius_hand_r);

	// Definition of contacts
	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s1_r;
	SmoothSphereHalfSpaceForce_s1_r = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s1_r", *calcn_r, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s1_r_location(0.00201451125786197447, -0.01000000000000000021, -0.00405444079003623292);
	SmoothSphereHalfSpaceForce_s1_r->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s1_r_location);
	double SmoothSphereHalfSpaceForce_s1_r_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s1_r->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s1_r_radius );
	SmoothSphereHalfSpaceForce_s1_r->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s1_r->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s1_r->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s1_r->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s1_r->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s1_r->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s1_r->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s1_r->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s1_r->connectSocket_sphere_frame(*calcn_r);
	SmoothSphereHalfSpaceForce_s1_r->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s1_r);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s2_r;
	SmoothSphereHalfSpaceForce_s2_r = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s2_r", *calcn_r, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s2_r_location(0.15723369200428286008, -0.01000000000000000021, -0.03042541204031325858);
	SmoothSphereHalfSpaceForce_s2_r->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s2_r_location);
	double SmoothSphereHalfSpaceForce_s2_r_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s2_r->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s2_r_radius );
	SmoothSphereHalfSpaceForce_s2_r->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s2_r->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s2_r->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s2_r->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s2_r->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s2_r->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s2_r->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s2_r->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s2_r->connectSocket_sphere_frame(*calcn_r);
	SmoothSphereHalfSpaceForce_s2_r->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s2_r);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s3_r;
	SmoothSphereHalfSpaceForce_s3_r = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s3_r", *calcn_r, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s3_r_location(0.14093114398356762496, -0.01000000000000000021, 0.05471497262864989664);
	SmoothSphereHalfSpaceForce_s3_r->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s3_r_location);
	double SmoothSphereHalfSpaceForce_s3_r_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s3_r->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s3_r_radius );
	SmoothSphereHalfSpaceForce_s3_r->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s3_r->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s3_r->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s3_r->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s3_r->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s3_r->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s3_r->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s3_r->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s3_r->connectSocket_sphere_frame(*calcn_r);
	SmoothSphereHalfSpaceForce_s3_r->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s3_r);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s4_r;
	SmoothSphereHalfSpaceForce_s4_r = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s4_r", *calcn_r, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s4_r_location(0.07018379790352752279, -0.01000000000000000021, 0.02793607986319752295);
	SmoothSphereHalfSpaceForce_s4_r->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s4_r_location);
	double SmoothSphereHalfSpaceForce_s4_r_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s4_r->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s4_r_radius );
	SmoothSphereHalfSpaceForce_s4_r->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s4_r->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s4_r->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s4_r->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s4_r->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s4_r->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s4_r->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s4_r->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s4_r->connectSocket_sphere_frame(*calcn_r);
	SmoothSphereHalfSpaceForce_s4_r->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s4_r);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s5_r;
	SmoothSphereHalfSpaceForce_s5_r = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s5_r", *toes_r, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s5_r_location(0.06357740011173836081, -0.01000000000000000021, -0.01987886062171620796);
	SmoothSphereHalfSpaceForce_s5_r->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s5_r_location);
	double SmoothSphereHalfSpaceForce_s5_r_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s5_r->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s5_r_radius );
	SmoothSphereHalfSpaceForce_s5_r->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s5_r->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s5_r->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s5_r->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s5_r->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s5_r->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s5_r->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s5_r->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s5_r->connectSocket_sphere_frame(*toes_r);
	SmoothSphereHalfSpaceForce_s5_r->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s5_r);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s6_r;
	SmoothSphereHalfSpaceForce_s6_r = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s6_r", *toes_r, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s6_r_location(0.04768305008380377408, -0.01000000000000000021, 0.06554507482174866395);
	SmoothSphereHalfSpaceForce_s6_r->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s6_r_location);
	double SmoothSphereHalfSpaceForce_s6_r_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s6_r->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s6_r_radius );
	SmoothSphereHalfSpaceForce_s6_r->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s6_r->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s6_r->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s6_r->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s6_r->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s6_r->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s6_r->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s6_r->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s6_r->connectSocket_sphere_frame(*toes_r);
	SmoothSphereHalfSpaceForce_s6_r->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s6_r);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s1_l;
	SmoothSphereHalfSpaceForce_s1_l = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s1_l", *calcn_l, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s1_l_location(0.00201451125786197447, -0.01000000000000000021, 0.00405444079003623292);
	SmoothSphereHalfSpaceForce_s1_l->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s1_l_location);
	double SmoothSphereHalfSpaceForce_s1_l_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s1_l->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s1_l_radius );
	SmoothSphereHalfSpaceForce_s1_l->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s1_l->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s1_l->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s1_l->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s1_l->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s1_l->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s1_l->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s1_l->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s1_l->connectSocket_sphere_frame(*calcn_l);
	SmoothSphereHalfSpaceForce_s1_l->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s1_l);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s2_l;
	SmoothSphereHalfSpaceForce_s2_l = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s2_l", *calcn_l, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s2_l_location(0.15723369200428286008, -0.01000000000000000021, 0.03042541204031325858);
	SmoothSphereHalfSpaceForce_s2_l->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s2_l_location);
	double SmoothSphereHalfSpaceForce_s2_l_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s2_l->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s2_l_radius );
	SmoothSphereHalfSpaceForce_s2_l->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s2_l->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s2_l->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s2_l->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s2_l->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s2_l->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s2_l->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s2_l->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s2_l->connectSocket_sphere_frame(*calcn_l);
	SmoothSphereHalfSpaceForce_s2_l->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s2_l);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s3_l;
	SmoothSphereHalfSpaceForce_s3_l = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s3_l", *calcn_l, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s3_l_location(0.14093114398356762496, -0.01000000000000000021, -0.05471497262864989664);
	SmoothSphereHalfSpaceForce_s3_l->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s3_l_location);
	double SmoothSphereHalfSpaceForce_s3_l_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s3_l->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s3_l_radius );
	SmoothSphereHalfSpaceForce_s3_l->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s3_l->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s3_l->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s3_l->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s3_l->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s3_l->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s3_l->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s3_l->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s3_l->connectSocket_sphere_frame(*calcn_l);
	SmoothSphereHalfSpaceForce_s3_l->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s3_l);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s4_l;
	SmoothSphereHalfSpaceForce_s4_l = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s4_l", *calcn_l, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s4_l_location(0.07018379790352752279, -0.01000000000000000021, -0.02793607986319752295);
	SmoothSphereHalfSpaceForce_s4_l->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s4_l_location);
	double SmoothSphereHalfSpaceForce_s4_l_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s4_l->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s4_l_radius );
	SmoothSphereHalfSpaceForce_s4_l->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s4_l->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s4_l->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s4_l->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s4_l->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s4_l->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s4_l->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s4_l->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s4_l->connectSocket_sphere_frame(*calcn_l);
	SmoothSphereHalfSpaceForce_s4_l->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s4_l);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s5_l;
	SmoothSphereHalfSpaceForce_s5_l = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s5_l", *toes_l, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s5_l_location(0.06357740011173836081, -0.01000000000000000021, 0.01987886062171620796);
	SmoothSphereHalfSpaceForce_s5_l->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s5_l_location);
	double SmoothSphereHalfSpaceForce_s5_l_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s5_l->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s5_l_radius );
	SmoothSphereHalfSpaceForce_s5_l->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s5_l->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s5_l->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s5_l->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s5_l->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s5_l->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s5_l->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s5_l->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s5_l->connectSocket_sphere_frame(*toes_l);
	SmoothSphereHalfSpaceForce_s5_l->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s5_l);

	OpenSim::SmoothSphereHalfSpaceForce* SmoothSphereHalfSpaceForce_s6_l;
	SmoothSphereHalfSpaceForce_s6_l = new SmoothSphereHalfSpaceForce("SmoothSphereHalfSpaceForce_s6_l", *toes_l, model->getGround());
	Vec3 SmoothSphereHalfSpaceForce_s6_l_location(0.04768305008380377408, -0.01000000000000000021, -0.06554507482174866395);
	SmoothSphereHalfSpaceForce_s6_l->set_contact_sphere_location(SmoothSphereHalfSpaceForce_s6_l_location);
	double SmoothSphereHalfSpaceForce_s6_l_radius = (0.03200000000000000067);
	SmoothSphereHalfSpaceForce_s6_l->set_contact_sphere_radius(SmoothSphereHalfSpaceForce_s6_l_radius );
	SmoothSphereHalfSpaceForce_s6_l->set_contact_half_space_location(Vec3(0.00000000000000000000, 0.00000000000000000000, 0.00000000000000000000));
	SmoothSphereHalfSpaceForce_s6_l->set_contact_half_space_orientation(Vec3(0.00000000000000000000, 0.00000000000000000000, -1.57079632679489655800));
	SmoothSphereHalfSpaceForce_s6_l->set_stiffness(1000000.00000000000000000000);
	SmoothSphereHalfSpaceForce_s6_l->set_dissipation(2.00000000000000000000);
	SmoothSphereHalfSpaceForce_s6_l->set_static_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s6_l->set_dynamic_friction(0.80000000000000004441);
	SmoothSphereHalfSpaceForce_s6_l->set_viscous_friction(0.50000000000000000000);
	SmoothSphereHalfSpaceForce_s6_l->set_transition_velocity(0.20000000000000001110);
	SmoothSphereHalfSpaceForce_s6_l->connectSocket_sphere_frame(*toes_l);
	SmoothSphereHalfSpaceForce_s6_l->connectSocket_half_space_frame(model->getGround());
	model->addComponent(SmoothSphereHalfSpaceForce_s6_l);

	// Initialize system.
	SimTK::State* state;
	state = new State(model->initSystem());

	// Read inputs.
	std::vector<T> x(arg[0], arg[0] + NX);
	std::vector<T> u(arg[1], arg[1] + NU);

	// States and controls.
	T ua[NU];
	Vector QsUs(NX);
	/// States
	for (int i = 0; i < NX; ++i) QsUs[i] = x[i];
	/// Controls
	/// OpenSim and Simbody have different state orders.
	auto indicesOSInSimbody = getIndicesOSInSimbody(*model);
	for (int i = 0; i < NU; ++i) ua[i] = u[indicesOSInSimbody[i]];

	// Set state variables and realize.
	model->setStateVariableValues(*state, QsUs);
	model->realizeVelocity(*state);

	// Compute residual forces.
	/// Set appliedMobilityForces (# mobilities).
	Vector appliedMobilityForces(nCoordinates);
	appliedMobilityForces.setToZero();
	/// Set appliedBodyForces (# bodies + ground).
	Vector_<SpatialVec> appliedBodyForces;
	int nbodies = model->getBodySet().getSize() + 1;
	appliedBodyForces.resize(nbodies);
	appliedBodyForces.setToZero();
	/// Set gravity.
	Vec3 gravity(0);
	gravity[1] = -9.80664999999999942304;
	/// Add weights to appliedBodyForces.
	for (int i = 0; i < model->getBodySet().getSize(); ++i) {
		model->getMatterSubsystem().addInStationForce(*state,
		model->getBodySet().get(i).getMobilizedBodyIndex(),
		model->getBodySet().get(i).getMassCenter(),
		model->getBodySet().get(i).getMass()*gravity, appliedBodyForces);
	}
	/// Add contact forces to appliedBodyForces.
	Array<osim_double_adouble> Force_0 = SmoothSphereHalfSpaceForce_s1_r->getRecordValues(*state);
	SpatialVec GRF_0;
	GRF_0[0] = Vec3(Force_0[3], Force_0[4], Force_0[5]);
	GRF_0[1] = Vec3(Force_0[0], Force_0[1], Force_0[2]);
	int c_idx_0 = model->getBodySet().get("calcn_r").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_0] += GRF_0;

	Array<osim_double_adouble> Force_1 = SmoothSphereHalfSpaceForce_s2_r->getRecordValues(*state);
	SpatialVec GRF_1;
	GRF_1[0] = Vec3(Force_1[3], Force_1[4], Force_1[5]);
	GRF_1[1] = Vec3(Force_1[0], Force_1[1], Force_1[2]);
	int c_idx_1 = model->getBodySet().get("calcn_r").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_1] += GRF_1;

	Array<osim_double_adouble> Force_2 = SmoothSphereHalfSpaceForce_s3_r->getRecordValues(*state);
	SpatialVec GRF_2;
	GRF_2[0] = Vec3(Force_2[3], Force_2[4], Force_2[5]);
	GRF_2[1] = Vec3(Force_2[0], Force_2[1], Force_2[2]);
	int c_idx_2 = model->getBodySet().get("calcn_r").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_2] += GRF_2;

	Array<osim_double_adouble> Force_3 = SmoothSphereHalfSpaceForce_s4_r->getRecordValues(*state);
	SpatialVec GRF_3;
	GRF_3[0] = Vec3(Force_3[3], Force_3[4], Force_3[5]);
	GRF_3[1] = Vec3(Force_3[0], Force_3[1], Force_3[2]);
	int c_idx_3 = model->getBodySet().get("calcn_r").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_3] += GRF_3;

	Array<osim_double_adouble> Force_4 = SmoothSphereHalfSpaceForce_s5_r->getRecordValues(*state);
	SpatialVec GRF_4;
	GRF_4[0] = Vec3(Force_4[3], Force_4[4], Force_4[5]);
	GRF_4[1] = Vec3(Force_4[0], Force_4[1], Force_4[2]);
	int c_idx_4 = model->getBodySet().get("toes_r").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_4] += GRF_4;

	Array<osim_double_adouble> Force_5 = SmoothSphereHalfSpaceForce_s6_r->getRecordValues(*state);
	SpatialVec GRF_5;
	GRF_5[0] = Vec3(Force_5[3], Force_5[4], Force_5[5]);
	GRF_5[1] = Vec3(Force_5[0], Force_5[1], Force_5[2]);
	int c_idx_5 = model->getBodySet().get("toes_r").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_5] += GRF_5;

	Array<osim_double_adouble> Force_6 = SmoothSphereHalfSpaceForce_s1_l->getRecordValues(*state);
	SpatialVec GRF_6;
	GRF_6[0] = Vec3(Force_6[3], Force_6[4], Force_6[5]);
	GRF_6[1] = Vec3(Force_6[0], Force_6[1], Force_6[2]);
	int c_idx_6 = model->getBodySet().get("calcn_l").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_6] += GRF_6;

	Array<osim_double_adouble> Force_7 = SmoothSphereHalfSpaceForce_s2_l->getRecordValues(*state);
	SpatialVec GRF_7;
	GRF_7[0] = Vec3(Force_7[3], Force_7[4], Force_7[5]);
	GRF_7[1] = Vec3(Force_7[0], Force_7[1], Force_7[2]);
	int c_idx_7 = model->getBodySet().get("calcn_l").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_7] += GRF_7;

	Array<osim_double_adouble> Force_8 = SmoothSphereHalfSpaceForce_s3_l->getRecordValues(*state);
	SpatialVec GRF_8;
	GRF_8[0] = Vec3(Force_8[3], Force_8[4], Force_8[5]);
	GRF_8[1] = Vec3(Force_8[0], Force_8[1], Force_8[2]);
	int c_idx_8 = model->getBodySet().get("calcn_l").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_8] += GRF_8;

	Array<osim_double_adouble> Force_9 = SmoothSphereHalfSpaceForce_s4_l->getRecordValues(*state);
	SpatialVec GRF_9;
	GRF_9[0] = Vec3(Force_9[3], Force_9[4], Force_9[5]);
	GRF_9[1] = Vec3(Force_9[0], Force_9[1], Force_9[2]);
	int c_idx_9 = model->getBodySet().get("calcn_l").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_9] += GRF_9;

	Array<osim_double_adouble> Force_10 = SmoothSphereHalfSpaceForce_s5_l->getRecordValues(*state);
	SpatialVec GRF_10;
	GRF_10[0] = Vec3(Force_10[3], Force_10[4], Force_10[5]);
	GRF_10[1] = Vec3(Force_10[0], Force_10[1], Force_10[2]);
	int c_idx_10 = model->getBodySet().get("toes_l").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_10] += GRF_10;

	Array<osim_double_adouble> Force_11 = SmoothSphereHalfSpaceForce_s6_l->getRecordValues(*state);
	SpatialVec GRF_11;
	GRF_11[0] = Vec3(Force_11[3], Force_11[4], Force_11[5]);
	GRF_11[1] = Vec3(Force_11[0], Force_11[1], Force_11[2]);
	int c_idx_11 = model->getBodySet().get("toes_l").getMobilizedBodyIndex();
	appliedBodyForces[c_idx_11] += GRF_11;

	/// knownUdot.
	Vector knownUdot(nCoordinates);
	knownUdot.setToZero();
	for (int i = 0; i < nCoordinates; ++i) knownUdot[i] = ua[i];
	/// Calculate residual forces.
	Vector residualMobilityForces(nCoordinates);
	residualMobilityForces.setToZero();
	model->getMatterSubsystem().calcResidualForceIgnoringConstraints(*state,
			appliedMobilityForces, appliedBodyForces, knownUdot, residualMobilityForces);

	/// Ground reaction forces and moments.
	Vec3 GRF_r(0), GRF_l(0), GRM_r(0), GRM_l(0);
	Vec3 normal(0, 1, 0);
	SimTK::Transform TR_GB_calcn_r = calcn_r->getMobilizedBody().getBodyTransform(*state);
	Vec3 SmoothSphereHalfSpaceForce_s1_r_location_G = calcn_r->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s1_r_location);
	Vec3 SmoothSphereHalfSpaceForce_s1_r_locationCP_G = SmoothSphereHalfSpaceForce_s1_r_location_G - SmoothSphereHalfSpaceForce_s1_r_radius * normal;
	Vec3 locationCP_G_adj_0 = SmoothSphereHalfSpaceForce_s1_r_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s1_r_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s1_r_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_0, *calcn_r);
	Vec3 GRM_0 = (TR_GB_calcn_r*SmoothSphereHalfSpaceForce_s1_r_locationCP_B) % GRF_0[1];
	GRF_r += GRF_0[1];
	GRM_r += GRM_0;

	Vec3 SmoothSphereHalfSpaceForce_s2_r_location_G = calcn_r->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s2_r_location);
	Vec3 SmoothSphereHalfSpaceForce_s2_r_locationCP_G = SmoothSphereHalfSpaceForce_s2_r_location_G - SmoothSphereHalfSpaceForce_s2_r_radius * normal;
	Vec3 locationCP_G_adj_1 = SmoothSphereHalfSpaceForce_s2_r_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s2_r_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s2_r_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_1, *calcn_r);
	Vec3 GRM_1 = (TR_GB_calcn_r*SmoothSphereHalfSpaceForce_s2_r_locationCP_B) % GRF_1[1];
	GRF_r += GRF_1[1];
	GRM_r += GRM_1;

	Vec3 SmoothSphereHalfSpaceForce_s3_r_location_G = calcn_r->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s3_r_location);
	Vec3 SmoothSphereHalfSpaceForce_s3_r_locationCP_G = SmoothSphereHalfSpaceForce_s3_r_location_G - SmoothSphereHalfSpaceForce_s3_r_radius * normal;
	Vec3 locationCP_G_adj_2 = SmoothSphereHalfSpaceForce_s3_r_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s3_r_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s3_r_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_2, *calcn_r);
	Vec3 GRM_2 = (TR_GB_calcn_r*SmoothSphereHalfSpaceForce_s3_r_locationCP_B) % GRF_2[1];
	GRF_r += GRF_2[1];
	GRM_r += GRM_2;

	Vec3 SmoothSphereHalfSpaceForce_s4_r_location_G = calcn_r->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s4_r_location);
	Vec3 SmoothSphereHalfSpaceForce_s4_r_locationCP_G = SmoothSphereHalfSpaceForce_s4_r_location_G - SmoothSphereHalfSpaceForce_s4_r_radius * normal;
	Vec3 locationCP_G_adj_3 = SmoothSphereHalfSpaceForce_s4_r_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s4_r_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s4_r_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_3, *calcn_r);
	Vec3 GRM_3 = (TR_GB_calcn_r*SmoothSphereHalfSpaceForce_s4_r_locationCP_B) % GRF_3[1];
	GRF_r += GRF_3[1];
	GRM_r += GRM_3;

	SimTK::Transform TR_GB_toes_r = toes_r->getMobilizedBody().getBodyTransform(*state);
	Vec3 SmoothSphereHalfSpaceForce_s5_r_location_G = toes_r->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s5_r_location);
	Vec3 SmoothSphereHalfSpaceForce_s5_r_locationCP_G = SmoothSphereHalfSpaceForce_s5_r_location_G - SmoothSphereHalfSpaceForce_s5_r_radius * normal;
	Vec3 locationCP_G_adj_4 = SmoothSphereHalfSpaceForce_s5_r_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s5_r_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s5_r_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_4, *toes_r);
	Vec3 GRM_4 = (TR_GB_toes_r*SmoothSphereHalfSpaceForce_s5_r_locationCP_B) % GRF_4[1];
	GRF_r += GRF_4[1];
	GRM_r += GRM_4;

	Vec3 SmoothSphereHalfSpaceForce_s6_r_location_G = toes_r->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s6_r_location);
	Vec3 SmoothSphereHalfSpaceForce_s6_r_locationCP_G = SmoothSphereHalfSpaceForce_s6_r_location_G - SmoothSphereHalfSpaceForce_s6_r_radius * normal;
	Vec3 locationCP_G_adj_5 = SmoothSphereHalfSpaceForce_s6_r_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s6_r_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s6_r_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_5, *toes_r);
	Vec3 GRM_5 = (TR_GB_toes_r*SmoothSphereHalfSpaceForce_s6_r_locationCP_B) % GRF_5[1];
	GRF_r += GRF_5[1];
	GRM_r += GRM_5;

	SimTK::Transform TR_GB_calcn_l = calcn_l->getMobilizedBody().getBodyTransform(*state);
	Vec3 SmoothSphereHalfSpaceForce_s1_l_location_G = calcn_l->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s1_l_location);
	Vec3 SmoothSphereHalfSpaceForce_s1_l_locationCP_G = SmoothSphereHalfSpaceForce_s1_l_location_G - SmoothSphereHalfSpaceForce_s1_l_radius * normal;
	Vec3 locationCP_G_adj_6 = SmoothSphereHalfSpaceForce_s1_l_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s1_l_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s1_l_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_6, *calcn_l);
	Vec3 GRM_6 = (TR_GB_calcn_l*SmoothSphereHalfSpaceForce_s1_l_locationCP_B) % GRF_6[1];
	GRF_l += GRF_6[1];
	GRM_l += GRM_6;

	Vec3 SmoothSphereHalfSpaceForce_s2_l_location_G = calcn_l->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s2_l_location);
	Vec3 SmoothSphereHalfSpaceForce_s2_l_locationCP_G = SmoothSphereHalfSpaceForce_s2_l_location_G - SmoothSphereHalfSpaceForce_s2_l_radius * normal;
	Vec3 locationCP_G_adj_7 = SmoothSphereHalfSpaceForce_s2_l_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s2_l_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s2_l_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_7, *calcn_l);
	Vec3 GRM_7 = (TR_GB_calcn_l*SmoothSphereHalfSpaceForce_s2_l_locationCP_B) % GRF_7[1];
	GRF_l += GRF_7[1];
	GRM_l += GRM_7;

	Vec3 SmoothSphereHalfSpaceForce_s3_l_location_G = calcn_l->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s3_l_location);
	Vec3 SmoothSphereHalfSpaceForce_s3_l_locationCP_G = SmoothSphereHalfSpaceForce_s3_l_location_G - SmoothSphereHalfSpaceForce_s3_l_radius * normal;
	Vec3 locationCP_G_adj_8 = SmoothSphereHalfSpaceForce_s3_l_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s3_l_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s3_l_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_8, *calcn_l);
	Vec3 GRM_8 = (TR_GB_calcn_l*SmoothSphereHalfSpaceForce_s3_l_locationCP_B) % GRF_8[1];
	GRF_l += GRF_8[1];
	GRM_l += GRM_8;

	Vec3 SmoothSphereHalfSpaceForce_s4_l_location_G = calcn_l->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s4_l_location);
	Vec3 SmoothSphereHalfSpaceForce_s4_l_locationCP_G = SmoothSphereHalfSpaceForce_s4_l_location_G - SmoothSphereHalfSpaceForce_s4_l_radius * normal;
	Vec3 locationCP_G_adj_9 = SmoothSphereHalfSpaceForce_s4_l_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s4_l_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s4_l_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_9, *calcn_l);
	Vec3 GRM_9 = (TR_GB_calcn_l*SmoothSphereHalfSpaceForce_s4_l_locationCP_B) % GRF_9[1];
	GRF_l += GRF_9[1];
	GRM_l += GRM_9;

	SimTK::Transform TR_GB_toes_l = toes_l->getMobilizedBody().getBodyTransform(*state);
	Vec3 SmoothSphereHalfSpaceForce_s5_l_location_G = toes_l->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s5_l_location);
	Vec3 SmoothSphereHalfSpaceForce_s5_l_locationCP_G = SmoothSphereHalfSpaceForce_s5_l_location_G - SmoothSphereHalfSpaceForce_s5_l_radius * normal;
	Vec3 locationCP_G_adj_10 = SmoothSphereHalfSpaceForce_s5_l_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s5_l_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s5_l_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_10, *toes_l);
	Vec3 GRM_10 = (TR_GB_toes_l*SmoothSphereHalfSpaceForce_s5_l_locationCP_B) % GRF_10[1];
	GRF_l += GRF_10[1];
	GRM_l += GRM_10;

	Vec3 SmoothSphereHalfSpaceForce_s6_l_location_G = toes_l->findStationLocationInGround(*state, SmoothSphereHalfSpaceForce_s6_l_location);
	Vec3 SmoothSphereHalfSpaceForce_s6_l_locationCP_G = SmoothSphereHalfSpaceForce_s6_l_location_G - SmoothSphereHalfSpaceForce_s6_l_radius * normal;
	Vec3 locationCP_G_adj_11 = SmoothSphereHalfSpaceForce_s6_l_locationCP_G - 0.5*SmoothSphereHalfSpaceForce_s6_l_locationCP_G[1] * normal;
	Vec3 SmoothSphereHalfSpaceForce_s6_l_locationCP_B = model->getGround().findStationLocationInAnotherFrame(*state, locationCP_G_adj_11, *toes_l);
	Vec3 GRM_11 = (TR_GB_toes_l*SmoothSphereHalfSpaceForce_s6_l_locationCP_B) % GRF_11[1];
	GRF_l += GRF_11[1];
	GRM_l += GRM_11;


	/// Residual forces.
	/// OpenSim and Simbody have different state orders so we need to adjust
	auto indicesSimbodyInOS = getIndicesSimbodyInOS(*model);
	for (int i = 0; i < NU; ++i) res[0][i] =
			value<T>(residualMobilityForces[indicesSimbodyInOS[i]]);

	int nc = 3;
	for (int i = 0; i < nc; ++i) res[0][i + NU + 0 * nc] = value<T>(GRF_r[i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 1 * nc] = value<T>(GRF_l[i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 2 * nc] = value<T>(GRM_r[i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 3 * nc] = value<T>(GRM_l[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 4 * nc] = value<T>(GRF_0[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 5 * nc] = value<T>(locationCP_G_adj_0[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 6 * nc] = value<T>(GRF_1[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 7 * nc] = value<T>(locationCP_G_adj_1[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 8 * nc] = value<T>(GRF_2[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 9 * nc] = value<T>(locationCP_G_adj_2[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 10 * nc] = value<T>(GRF_3[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 11 * nc] = value<T>(locationCP_G_adj_3[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 12 * nc] = value<T>(GRF_4[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 13 * nc] = value<T>(locationCP_G_adj_4[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 14 * nc] = value<T>(GRF_5[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 15 * nc] = value<T>(locationCP_G_adj_5[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 16 * nc] = value<T>(GRF_6[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 17 * nc] = value<T>(locationCP_G_adj_6[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 18 * nc] = value<T>(GRF_7[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 19 * nc] = value<T>(locationCP_G_adj_7[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 20 * nc] = value<T>(GRF_8[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 21 * nc] = value<T>(locationCP_G_adj_8[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 22 * nc] = value<T>(GRF_9[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 23 * nc] = value<T>(locationCP_G_adj_9[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 24 * nc] = value<T>(GRF_10[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 25 * nc] = value<T>(locationCP_G_adj_10[i]);

	for (int i = 0; i < nc; ++i) res[0][i + NU + 26 * nc] = value<T>(GRF_11[1][i]);
	for (int i = 0; i < nc; ++i) res[0][i + NU + 27 * nc] = value<T>(locationCP_G_adj_11[i]);

	return 0;
}

int main() {
	Recorder x[NX];
	Recorder u[NU];
	Recorder tau[NR];
	for (int i = 0; i < NX; ++i) x[i] <<= 0;
	for (int i = 0; i < NU; ++i) u[i] <<= 0;
	const Recorder* Recorder_arg[n_in] = { x,u };
	Recorder* Recorder_res[n_out] = { tau };
	F_generic<Recorder>(Recorder_arg, Recorder_res);
	double res[NR];
	for (int i = 0; i < NR; ++i) Recorder_res[0][i] >>= res[i];
	Recorder::stop_recording();
	return 0;
}
