#ifndef AFXL_H_M1RFGRE5
#define AFXL_H_M1RFGRE5

#include "umundo/prediction/SPA.h"

namespace umundo {
	
class AFxL : public SPA {
public:
  AFxL(NGramModel* model) : SPA(model) {}

  virtual map<string, float> getPredictions(list<string> history);
  vector<float> _factors;
};
	
}

#endif /* end of include guard: AFXL_H_M1RFGRE5 */
