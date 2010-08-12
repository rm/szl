// Copyright 2010 Google Inc.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//      http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------------

#include <string>
#include <vector>

#include "public/porting.h"
#include "public/logging.h"

#include "public/szlencoder.h"
#include "public/szldecoder.h"
#include "public/szlresults.h"
#include "public/szlvalue.h"

class SzlSetResults: public SzlResults {
 public:
  static SzlResults *Create(const SzlType& type, string *error) {
    return new SzlSetResults(type);
  }

  explicit SzlSetResults(const SzlType& type)
    : ops_(type.element()->type()),
      maxElems_(type.param()),
      totElems_(0) {
  }

  // Check if the mill type is a valid instance of this table kind.
  // If not, a reason is returned in error.
  // We already know all indices are valid, as are the types for the
  // element and the weight, which is present iff it's needed.
  static bool Validate(const SzlType& type, string* error) {
    return true;
  }

  // Retrieve the properties for this kind of table.
  static void Props(const char* kind, SzlType::TableProperties* props) {
    props->name = kind;
    props->has_param = true;
    props->has_weight = false;
  }

  // Fill in fields with the non-index fields in the result.
  // Type is valid and of the appropriate kind for this table.
  static void ElemFields(const SzlType &t, vector<SzlField>* fields) {
    AppendField(t.element(), kValueLabel, fields);
  }

  // Read the string, just like SzlSetEntry::Merge
  // Just like, but not quite enough to combine the code...
  virtual bool ParseFromString(const string& val) {
    elems_.clear();
    totElems_ = 0;
    if (val.empty())
      return true;
    
    SzlDecoder dec(val.data(), val.size());
    int64 delta, nvals;
    if (!dec.GetInt(&delta) || !dec.GetInt(&nvals))
      return false;
    if (nvals > maxElems_+1 || nvals < 0)  // simple validity test
      return false;
    // is the string valid?
    for (int i = 0; i < nvals; ++i)
      if (!ops_.Skip(&dec))
        return false;
    if (!dec.done())
      return false;
    dec.Restart();
    CHECK(dec.Skip(SzlType::INT));
    CHECK(dec.Skip(SzlType::INT));
    

    // Pick up each element.
    // They are SzlEncoded values of our set's element type.
    // We want to leave them in their encoded format,
    // but use SzlValue's ability to parse instances of complex SzlTypes.
    for (int i = 0; i < nvals; i++) {
      // Record the starting position of the next value.
      unsigned const char* p = dec.position();

      // Skip past it.
      CHECK(ops_.Skip(&dec));

      // Now we know the end of the encoded value.
      // Add the entire encoded value.
      elems_.push_back(string(reinterpret_cast<const char*>(p),
                              dec.position() - p));
    }
    totElems_ = delta + nvals;  // keeping track of emits
    if (elems_.size() > maxElems_)
      elems_.clear();  // set overflowed, so don't report it
    return true;
  }

  virtual const vector<string> *Results() { return &elems_;}

  // Report the total elements added to the table.
  virtual int64 TotElems() const { return totElems_; }
  
 private:
  SzlOps ops_;                  // Operations on our element type, for parsing.
  vector<string> elems_;
  int maxElems_;
  int totElems_;
};

REGISTER_SZL_RESULTS(set, SzlSetResults);
