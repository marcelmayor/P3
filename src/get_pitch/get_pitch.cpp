/// @file

#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>

#include "wavfile_mono.h"
#include "pitch_analyzer.h"

#include "docopt.h"

#define FRAME_LEN   0.030 /* 30 ms. */
#define FRAME_SHIFT 0.015 /* 15 ms. */

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Detector 

Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version

Options:
    -h, --help  Show this screen
    --version   Show the version of the project

Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the detection:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0
)";

int main(int argc, const char *argv[]) {
	/// \TODO 
	///  Modify the program syntax and the call to **docopt()** in order to
	///  add options and arguments to the program.
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},	// array of arguments, without the program name
        true,    // show help if requested
        "2.0");  // version string

	std::string input_wav = args["<input-wav>"].asString();
	std::string output_txt = args["<output-txt>"].asString();

  // Read input sound file
  unsigned int rate;
  vector<float> x;
  if (readwav_mono(input_wav, rate, x) != 0) {
    cerr << "Error reading input file " << input_wav << " (" << strerror(errno) << ")\n";
    return -2;
  }

  int n_len = rate * FRAME_LEN;
  int n_shift = rate * FRAME_SHIFT;

  // Define analyzer
  PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::RECT, 50, 500);

  /// \TODO
  /// Preprocess the input signal in order to ease pitch estimation. For instance,
  /// central-clipping or low pass filtering may be used.
  /// \DONE Realizado
  float maxValue = 0.0F;
  for(unsigned int k = 0; k< x.size(); k++){ //Busqueda del máximo de la señal
    if(x[k]> maxValue)
    maxValue = x[k];

  }

  float thCC = 0.01*maxValue; //Umbral 1% max{x[n]}
  for(unsigned int k = 0; k < x.size(); k++){ //Función de recorte
    if (x[k] > thCC)
     x[k] -= thCC;

    else if (x[k] < -thCC)
    x[k] += thCC;

    else
    x[k] = 0;
  }

  
  // Iterate for each frame and save values in f0 vector
  vector<float>::iterator iX;
  vector<float> f0;
  for (iX = x.begin(); iX + n_len < x.end(); iX = iX + n_shift) {
    float f = analyzer(iX, iX + n_len);
    f0.push_back(f);
  }

  /// \TODO
  /// Postprocess the estimation in order to supress errors. For instance, a median filter
  /// or time-warping may be used.
  /// \DONE Realizado

   int Nfilter = 3;
   for (unsigned int i = (Nfilter-1)/2; i < f0.size() - (Nfilter-1)/2; ++i){
     float Cw[Nfilter];

     for (int j = 0; j < Nfilter; ++j)
 	      Cw[j] = f0[i - (Nfilter-1)/2 + j];

     for (int j = 0; j < Nfilter - (Nfilter-1)/2; ++j){
 	      int min = j;

 	      for (int k = j + 1; k < Nfilter; ++k)
 	        if (Cw[k] < Cw[min])
 	          min = k;

        const float temp = Cw[j];
        Cw[j] = Cw[min];
        Cw[min] = temp;
     }
     f0[i] = Cw[(Nfilter-1)/2];
   }



  // Write f0 contour into the output file
  ofstream os(output_txt);
  if (!os.good()) {
    cerr << "Error reading output file " << output_txt << " (" << strerror(errno) << ")\n";
    return -3;
  }

  os << 0 << '\n'; //pitch at t=0
  for (iX = f0.begin(); iX != f0.end(); ++iX) 
    os << *iX << '\n';
  os << 0 << '\n';//pitch at t=Dur

  return 0;
}
