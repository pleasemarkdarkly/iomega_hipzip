#include <iostream>
#include <string>
#include "memmap.h"

using namespace std;

string & make_basename(string & filename) {
  size_t pos = filename.rfind('/');
  if (pos != string::npos)
    filename.erase(0, filename.rfind('/')+1);

  return filename;
}

string & set_file_suffix(string & filename, string suffix) {
  size_t pos = filename.rfind('.');

  if (pos == string::npos) {
    filename += suffix;
  } else {
    filename.replace(pos, string::npos, suffix);
  }

  return filename;
}

int main(int argc, char **argv) {
  string ldi, h, destpath;
  if (argc < 2 || argc > 3 || (strcmp(argv[1],"--help")==0)) {
    clog << "Usage: " << endl
         << "  " << argv[0] << ": mltfile.mlt destpath" << endl
 	 << "  destpath is typically buildname_install/include/pkgconf " << endl
         << "  uses working directory if no directory is specified" << endl;
    return 1;
  }

  if (argc == 3) {
    destpath = argv[2];
    if (destpath[destpath.length()-1] != '/') destpath += '/';
  }

  ldi = destpath + set_file_suffix(make_basename(ldi=argv[1]),".ldi");

  h = destpath + set_file_suffix(make_basename(h=argv[1]),".h");

  mem_map map;
  map.set_map_size(0xffffffff);

  clog << "Reading " << argv[1] << endl;
  if(!map.load_memory_layout(argv[1])) {
    clog << "load_memory_layout failed" << endl;
    return 1;
  }

  clog << "Creating " << ldi << " and " << h << endl;
  if (!map.export_files(ldi, h)) {
    clog << "export_files failed" << endl;
    return 1;
  }

  return 0;
}
