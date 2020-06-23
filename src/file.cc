#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include "note.h"
#include "color.h"

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::to_string;
using std::transform;
using std::ofstream;
using std::ifstream;
using std::ios;
using std::min;
using std::max;

void saveFile(string path, mfile* file, const vector<colorRGB>& colorVecA, const vector<colorRGB>& colorVecB,
              bool colorByPart, bool drawLine, bool songTime, bool invertColor) {
  // force correct file extension
  transform(path.begin(), path.end(), path.begin(), ::tolower);
  if (path.substr(path.size() - 4) != ".mki") {
    path.append(".mki");
  }
 
  // ensure file is valid
  if (path.size() == 4) {
    cerr << "warn: saving file with no name" << endl;
  }

  // generate notes
  int noteCount = file->getNoteCount();
  note* notes = new note[noteCount];
  notes = file->getNotes();

  // start constructing the file
  ofstream output;
  output.open(path, ios::out | ios::binary);
  if(!output) {
    cerr<< "warn: unable to save file at path " << path <<endl;
    return;
  }
  
  //define control chars
  char separator = '|';
  char noteSeparator = '~';

  // file format begins with first byte '|' (0x01111100)
  output.write(&separator, sizeof(separator));
  
  // second byte encodes settings data
  // top four bits currently unused
  uint8_t setting = 0x00000000;
  setting += colorByPart;
  setting <<=1;
  setting += drawLine;
  setting <<=1;
  setting += songTime;
  setting <<=1;
  setting += invertColor;

  output.write(reinterpret_cast<char*>(&setting), sizeof(setting));

  // third byte encodes note count
  output.write(reinterpret_cast<char*>(&noteCount), sizeof(noteCount));

  // fourth byte encodes color count
  int colorCount = colorVecA.size();
  output.write(reinterpret_cast<char*>(&colorCount), sizeof(colorCount));
  
  // each color needs 4 bytes
  // byte 1: R
  // byte 2: G
  // byte 3: B
  // byte 4: A

  // beginning of colorA
  output.write(&separator, sizeof(separator));
  for (int i = 0; i < static_cast<int>(colorVecA.size()); i++) {
    Uint32 color = 0x00000000;
    color += static_cast<Uint8>(colorVecA[i].r);
    color <<= 8;
    color += static_cast<Uint8>(colorVecA[i].g);
    color <<= 8;
    color += static_cast<Uint8>(colorVecA[i].b);
    color <<=8;
    output.write(reinterpret_cast<char*>(&color), sizeof(color));
  }
  // beginning of colorB
  output.write(&separator, sizeof(separator));
  for (int i = 0; i < static_cast<int>(colorVecB.size()); i++) {
    Uint32 color = 0x00000000;
    color += static_cast<Uint8>(colorVecB[i].r);
    color <<= 8;
    color += static_cast<Uint8>(colorVecB[i].g);
    color <<= 8;
    color += static_cast<Uint8>(colorVecB[i].b);
    color <<=8;
    color += 0xFF;
    output.write(reinterpret_cast<char*>(&color), sizeof(color));
  }
  // end of color section
  output.write(&separator, sizeof(separator));

  // midi data encoding
  for (int i = 0; i < noteCount; i++) {
    // each note needs 20 bytes to encode: broken down as follows
    // 1 byte: track (uint8_t)
    // 2 bytes: tempo (uint16_t)
    // 8 bytes: duration (double)
    // 8 bytes: x (double)
    // 1 bytes: y (unsigned char)
    // note: this limits the amount of tracks to 256
    // note: this limits the Y value to 0 - 255
    // note: this limits the tempo to 0 - 65535

    if (notes[i].track > 255 || notes[i].y > 255) {
      cerr << "warn: file attributes exceed limits" << endl;
    }
    
    uint16_t tempoShort = static_cast<uint16_t>(notes[i].tempo);

    output.write(&noteSeparator, sizeof(noteSeparator));
    output.write(const_cast<char*>(reinterpret_cast<const char*>(&notes[i].track)), sizeof(uint8_t));
    output.write(const_cast<char*>(reinterpret_cast<const char*>(&tempoShort)), sizeof(uint16_t));
    output.write(const_cast<char*>(reinterpret_cast<const char*>(&notes[i].duration)), sizeof(double));
    output.write(const_cast<char*>(reinterpret_cast<const char*>(&notes[i].x)), sizeof(double));
    output.write(const_cast<char*>(reinterpret_cast<const char*>(&notes[i].y)), sizeof(uint8_t));

    cerr << "this is note " << i << endl;
    cerr << notes[i].track << endl;
    cerr << notes[i].tempo << endl;
    cerr << notes[i].duration << endl;
    cerr << notes[i].x << endl;
    cerr << notes[i].y << endl;
  }  

  // end save file
  output.close(); 
}

bool checkMKI(ifstream& file, string path) {
  // this function verifies the integrity of the MKI file
  char formatcheck = '\0';
  file.read(reinterpret_cast<char *>(&formatcheck), sizeof(formatcheck));
  cerr << formatcheck << endl;
  if (formatcheck != '|') {
    cerr << "error: invalid MKI file: " << path << "!" << endl;
    return false;
  }
  return true;
}

void loadFileMKI(string path, mfile*& input, vector<colorRGB>& colorVecA, vector<colorRGB> colorVecB, bool& colorByPart, bool& drawLine, bool& songTime, bool& invertColor){
  ifstream file;
  file.open(path, ios::in | ios::binary);
  if (!file) {
    cerr << "error: invalid file: " << path << "!" << endl;
    return;
  }
  
  // ensure this is actually an MKI file
  if (!checkMKI(file, path)) { return; }
  
  // read the next byte and set the bool values (in order)
  uint8_t boolValue = 0; 
  file.read(reinterpret_cast<char *>(&boolValue), sizeof(uint8_t));
  cerr << boolValue << endl;
  colorByPart = boolValue & 0x00001000;
  drawLine = boolValue & 0x00000100;
  songTime = boolValue & 0x00000010;
  invertColor = boolValue & 0x00000001;


  // read the note count at third byte
  file.read(reinterpret_cast<char *>(&input->noteCount), sizeof(int));
  cerr << input->noteCount << endl;

  // read the color count
  int colorCount = 0;
  file.read(reinterpret_cast<char *>(&colorCount), sizeof(int));
  cerr << colorCount << endl;

  cerr << file.gcount() << endl;
  
  // next byte must be a separator
  if (!checkMKI(file, path)) { return; }

  colorRGB col;
  
  // next 4 * colorCount bytes are the on colors
  for (int i = 0; i < colorCount; i++) {
    // read 4 bytes, store three in col, discard the alpha byte

    file.read(reinterpret_cast<char *>(&col.r), sizeof(uint8_t));
    file.read(reinterpret_cast<char *>(&col.g), sizeof(uint8_t));
    file.read(reinterpret_cast<char *>(&col.b), sizeof(uint8_t));

    // the alpha field doesn't actually do anything...
    file.seekg(1, ios::cur);

    colorVecA.push_back(col);
  }

  // between the colors there is a separator
  if (!checkMKI(file, path)) { return; }
  
  // next 4 * colorCount bytes are the off colors
  for (int i = 0; i < colorCount; i++) {
    // read 4 bytes, store three in col, discard the alpha byte

    file.read(reinterpret_cast<char *>(&col.r), sizeof(uint8_t));
    file.read(reinterpret_cast<char *>(&col.g), sizeof(uint8_t));
    file.read(reinterpret_cast<char *>(&col.b), sizeof(uint8_t));

    // the alpha field doesn't actually do anything...
    file.seekg(1, ios::cur);

    colorVecB.push_back(col);
  }
 
  // after the colors there is a separator
  if (!checkMKI(file, path)) { return; }

  // initialize the note array
  input->notes = new note[input->noteCount];

  // initialize misc. data
  input->timeScale = 1;
  
  for (int i = 0; i < input->noteCount; i++) {
    // read into array
    file.read(reinterpret_cast<char*>(&input->notes[i].track), sizeof(uint8_t));
    file.read(reinterpret_cast<char*>(&input->notes[i].tempo), sizeof(uint16_t));
    file.read(reinterpret_cast<char*>(&input->notes[i].duration), sizeof(double));
    file.read(reinterpret_cast<char*>(&input->notes[i].x), sizeof(double));
    file.read(reinterpret_cast<char*>(&input->notes[i].y), sizeof(uint8_t));
    
    // get misc. data
    if (i == 0) {
      input->noteMax = input->notes[0].y;
      input->noteMin = input->notes[0].y;
    }

    input->trackCount = max(input->trackCount, input->notes[i].track);

    input->noteMax = max(input->noteMax, input->notes[i].y);
    input->noteMin = max(input->noteMin, input->notes[i].y);

    if (i + 1 == input->noteCount) {
      input->lastTick = input->notes[i].x + input->notes[i].duration;
    }
  }

  // file processing is done
  file.close();
  return;
}
