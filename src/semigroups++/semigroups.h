/*
 * Semigroups++
 *
 * This file contains the Froidure-Pin algorithm for arbitrary semigroups. 
 *
 */

// TODO
//
// 1) bit flipping for reduced?
// 2) remove RecVecs
// 3) free stuff at the end
// 4) cache in trace
// 5) next_relation 
// 6) the other functionality of Semigroupe.
// 7) rename degree to element_size or something

#ifndef SEMIGROUPS_H
#define SEMIGROUPS_H

//#define NDEBUG
//#define DEBUG

#ifndef BATCH_SIZE
#define BATCH_SIZE 8192
#endif

#include "basics.h"
#include "elements.h"

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <assert.h>
#include <iostream>

class SemigroupBase { };

template <typename T>
class Semigroup : public SemigroupBase {
  
  typedef std::vector<size_t> Word;
  typedef std::pair<Word*, Word*> Relation;

  public:
    
    Semigroup& operator= (Semigroup const& copy) = delete;
    
    Semigroup (const Semigroup& copy, size_t nr_new_gens = 0) 
      : _degree(copy._degree),
        _elements(new std::vector<T*>()),
        _final(copy._final),
        _first(copy._first),
        _found_one(copy._found_one),
        _gens(),
        _genslookup(copy._genslookup),
        _id(static_cast<T*>(copy._id->copy())),
        _index(copy._index),
        _left(new RecVec<size_t>(copy._nrgens + nr_new_gens)),
        _lenindex(copy._lenindex),
        _map(),
        _nr(copy._nr),
        _nrgens(copy._nrgens),
        _nrrules(copy._nrrules),
        _pos(copy._pos),
        _pos_one(copy._pos_one),
        _prefix(copy._prefix),
        _reduced(RecVec<bool>(copy._nrgens + nr_new_gens)),
        _right(new RecVec<size_t>(copy._nrgens + nr_new_gens)),
        _suffix(copy._suffix),
        _wordlen(copy._wordlen)
    { 
      std::cout << "starting to copying C++ semigroup object!!!\n";
      for (size_t i = 0; i < _nrgens; i++) {
        _gens.push_back(static_cast<T*>(copy._gens.at(i)->copy()));
      }
      for (size_t i = 0; i < copy._elements->size(); i++) {
        _elements->push_back(static_cast<T*>(copy._elements->at(i)->copy()));
        _map.insert(std::make_pair(*_elements->back(), i));
      }
      _reduced.reserve((_nrgens + nr_new_gens) * _nr);
      _left->reserve((_nrgens + nr_new_gens) * _nr);
      _right->reserve((_nrgens + nr_new_gens) * _nr);
      for (size_t i = 0; i < _nr; i++) {
        for (size_t j = 0; j < _nrgens; j++) {
          _reduced.push_back(copy._reduced.get(i, j));
          _right->push_back(copy._right->get(i, j));
          _left->push_back(copy._left->get(i, j));
        }
        for (size_t j = 0; j < nr_new_gens; j++) {
          _reduced.push_back(false);
          _right->push_back(0);
          _left->push_back(0);
        }
      }
      _left->set_nrrows(_nr);
      _right->set_nrrows(_nr);
      _reduced.set_nrrows(_nr);
      std::cout << "finished copying C++ semigroup object!!!\n";
    }

    Semigroup (std::vector<T*> gens, size_t degree) : 
      _degree     (degree),
      _elements   (new std::vector<T*>()),
      _final      (),
      _first      (),    
      _found_one  (false),
      _gens       (gens), 
      _genslookup (),
      _index      (),
      _left       (new RecVec<size_t>(gens.size())),
      _lenindex   (), 
      _map        (), 
      _nr         (0), 
      _nrgens     (gens.size()),
      _nrrules    (0), 
      _pos        (0), 
      _pos_one    (0), 
      _prefix     (), 
      _reduced    (RecVec<bool>(gens.size())),
      _right      (new RecVec<size_t>(gens.size())),
      _suffix     (), 
      _wordlen    (0) // (length of the current word) - 1
    { 
      assert(_nrgens != 0);
      
      _lenindex.push_back(0);
      _id = static_cast<T*>(_gens.at(0)->identity());

      // init genslookup//TODO remove this.
      for (size_t i = 0; i < _nrgens; i++) {
        _genslookup.push_back(0);
      }

      // add the generators 
      for (size_t i = 0; i < _nrgens; i++) {
        T* x = _gens.at(i);
        auto it = _map.find(*x);
        if (it != _map.end()) { // duplicate generator
          _genslookup.at(i) = it->second; //TODO push_back here instead
          _nrrules++;
        } else {
          is_one(*x);
          _elements->push_back(static_cast<T*>(x->copy()));
          _first.push_back(i);
          _final.push_back(i);
          _genslookup.at(i) = _nr;
          _map.insert(std::make_pair(*_elements->back(), _nr));
          _prefix.push_back(_nr);
          _suffix.push_back(_nr);
          _nr++;
        }
      }
      expand();
    }

    ~Semigroup () {
      // FIXME duplicate generators are not deleted?
      delete _left;
      delete _right;
      for (T* x: *_elements) {
        x->delete_data();
        delete x;
      }
      delete _elements;
      _id->delete_data();
      delete _id;
    }

    size_t max_word_length () {
      if (_nr > _lenindex.back()) { 
        return _lenindex.size();
      } else {
        return _lenindex.size() - 1;
      }
    }

    size_t degree () {
      return _degree;
    }
   
    size_t nrgens () {
      return _gens.size();
    }
    
    std::vector<T*> gens () {
      return _gens;
    }
    
    bool is_done () {
      return (_pos >= _nr);
    }
    
    bool is_begun () {
      assert(_lenindex.size() > 1);
      return (_pos >= _lenindex.at(1));
    }

    size_t current_size () {
      return _elements->size();
    }
    
    size_t size (bool report) {
      enumerate(-1, report);
      return _elements->size();
    }
   
    size_t test_membership (T* x) {
      return (position(x) != (size_t) -1);
    }

    size_t position (T* x) {
      if (x->degree() != _degree) {
        return -1;
      }

      while (true) { 
        auto it = _map.find(*x);
        if (it != _map.end()) {
          return it->second;
        }
        if (is_done()) {
          return -1;
        }
        enumerate(_nr + 1); 
        // the _nr means we enumerate BATCH_SIZE more elements
      }
    }

    std::vector<T*>* elements (size_t limit) {
      enumerate(limit);
      return _elements;
    }
    
    size_t nrrules () {
      return _nrrules;
    }
    
    RecVec<size_t>* right_cayley_graph () {
      enumerate(-1);
      return _right;
    }
    
    RecVec<size_t>* left_cayley_graph () {
      enumerate(-1);
      return _left;
    }
    
    Word* trace (size_t pos) { // trace the spanning tree
      // caching the words seems to be slower ....
      /*if (_words.empty()) {
        _words.reserve(_elements.size());
        for (size_t i = 0; i < _elements.size(); i++) {
          _words.push_back(std::vector<size_t>());
        }
        for (size_t i = 0; i < _genslookup.size(); i++) {
          if (_words.at(_genslookup.at(i)).empty()) {
            _words.at(_genslookup.at(i)).push_back(i);
          }
        }
      }

      std::vector<size_t> word2;
      while (_words.at(pos).empty()) {
        word2.push_back(this->schreiergen(pos));
        pos = this->schreierpos(pos);
      }
      //word.push_back(_genslookup.at(pos));
      std::reverse(word2.begin(), word2.end());
      std::vector<size_t> word1(_words.at(pos));
      word1.insert(word1.end(), word2.begin(), word2.end());
      return word1;*/

      Word* word = new Word();
      while (pos >= _lenindex.at(1)) {
        word->push_back(_first.at(pos));
        pos = _suffix.at(pos);
      }
      word->push_back(_genslookup.at(pos));
      return word;
    }
    
    // TODO make this next_relation with gets handed in std::vector reference
    // and which just puts the [element-index, generators, element-index] into
    // the std::vector.
    std::vector<Relation>* relations () {
      enumerate(-1);
      std::vector<Relation>* relations = new std::vector<Relation>();
      int nr = (int) _nrrules;
      
      size_t tmp = 0;

      for (size_t i = 1; i < _gens.size(); i++) {
        if (_genslookup.at(i) <= _genslookup.at(i - 1)) {
          nr--;
          relations->push_back(make_relation(i, _genslookup.at(i)));
        }
      }
      std::cout << "nr of relations = " << relations->size() - tmp << "\n";
      tmp = relations->size();
      
      size_t i;
      for (i = 0; i < _lenindex.at(1); i++) {
        for (size_t j = 0; j < _reduced.nrcols(); j++) {
          if (!_reduced.get(i, j)) {
            nr--;
            relations->push_back(make_relation(i, j));
          }
        }
      }
      std::cout << "nr of relations = " << relations->size() - tmp << "\n";
      tmp = relations->size();
      
      for (; i < _reduced.nrrows(); i++) {
        for (size_t j = 0; j < _reduced.nrcols(); j++) {
          if (_reduced.get(_suffix.at(i), j) && !_reduced.get(i, j)) {
            nr--;
            relations->push_back(make_relation(i, j));
          }
        }
      }
      std::cout << "nr of relations = " << relations->size() - tmp << "\n";
      
      std::cout << "_nrrules = " << _nrrules << "\n";
      assert(nr == 0);
      return relations;
    }
   
    void enumerate (size_t limit) {
      enumerate(limit, false);
    }
    
    size_t simple_size () {
      T x(_degree, _gens.at(0)); 
      size_t report = 0;
      while (_pos < _nr) {
        for (size_t j = 0; j < _nrgens; j++) {
          x.redefine(_elements->at(_pos), _gens.at(j)); 
          auto it = _map.find(x); 
          if (it == _map.end()) {
            _elements->push_back(static_cast<T*>(x.copy()));
            _map.insert(std::make_pair(*_elements->back(), _nr));
            _nr++;
          }
        }
        _pos++;
        if (_nr > report + 10000) {
          report = _nr;
          std::cout << "found " << _nr << " elements so far\n";
        }
      }
      x.delete_data();
      return _nr;
    }

    void enumerate (size_t limit, bool report) {
      if (_pos >= _nr || limit <= _nr) return;
      limit = std::max(limit, _nr + BATCH_SIZE);
      
      std::cout << "C++ version\n";
      std::cout << "limit = " << limit << "\n";
      
      T x(_degree, _gens.at(0)); 
      // pass in sample object to, for example, pass on the semiring for
      // MatrixOverSemiring

      //multiply the generators by every generator
      if (_pos < _lenindex.at(1)) {
        while (_pos < _lenindex.at(1)) { 
          for (size_t j = 0; j < _nrgens; j++) {
            x.redefine(_elements->at(_pos), _gens.at(j)); 
            auto it = _map.find(x); 

            if (it != _map.end()) {
              _right->set(_pos, j, it->second);
              _nrrules++;
            } else {
              is_one(x);
              _elements->push_back(static_cast<T*>(x.copy()));
              _first.push_back(_first.at(_pos));
              _final.push_back(j);
              _map.insert(std::make_pair(*_elements->back(), _nr));
              _prefix.push_back(_pos);
              _reduced.set(_pos, j, true);
              _right->set(_pos, j, _nr);
              _suffix.push_back(_genslookup.at(j));
              _nr++;
            }
          }
          _pos++;
        }
        for (size_t i = 0; i < _pos; i++) { 
          size_t b = _final.at(i); 
          for (size_t j = 0; j < _nrgens; j++) { 
            _left->set(i, j, _right->get(_genslookup.at(j), b));
          }
        }
        _wordlen++;
        expand();
      }

      //multiply the words of length > 1 by every generator
      bool stop = (_nr >= limit);

      while (_pos < _nr && !stop) {
        while (_pos < _lenindex.at(_wordlen + 1) && !stop) {
          size_t b = _first.at(_pos);
          size_t s = _suffix.at(_pos); 
          for (size_t j = 0; j < _nrgens; j++) {
            if (!_reduced.get(s, j)) {
              size_t r = _right->get(s, j);
              if (_found_one && r == _pos_one) {
                _right->set(_pos, j, _genslookup.at(b));
              } else if (r >= _lenindex.at(1)) {
                _right->set(_pos, j, _right->get(_left->get(_prefix.at(r), b),
                                                 _final.at(r)));
              } else { // TODO it would be nice to get rid of this case somehow
                _right->set(_pos, j, _right->get(_genslookup.at(b), _final.at(r)));
              } 
            } else {
              x.redefine(_elements->at(_pos), _gens.at(j)); 
              auto it = _map.find(x); 

              if (it != _map.end()) {
                _right->set(_pos, j, it->second);
                _nrrules++;
              } else {
                is_one(x);
                _elements->push_back(static_cast<T*>(x.copy()));
                _first.push_back(b);
                _final.push_back(j);
                _map.insert(std::make_pair(*_elements->back(), _nr));
                _prefix.push_back(_pos);
                _reduced.set(_pos, j, true);
                _right->set(_pos, j, _nr);
                _suffix.push_back(_right->get(s, j));
                _nr++;
                stop = (_nr >= limit);
              }
            }
          } // finished applying gens to <_elements->at(_pos)>
          _pos++;
        } // finished words of length <wordlen> + 1
        if (_pos > _nr || _pos == _lenindex.at(_wordlen + 1)) {
          for (size_t i = _lenindex.at(_wordlen); i < _pos; i++) { 
            size_t p = _prefix.at(i);
            size_t b = _final.at(i); 
            for (size_t j = 0; j < _nrgens; j++) { 
              _left->set(i, j, _right->get(_left->get(p, j), b));
            }
          }
          _wordlen++;
          expand();
        }
        if (report) {
          std::cout << "found " << _nr << " elements, ";
          std::cout << _nrrules << " rules, ";
          std::cout << "max word length " << _wordlen + 1 << ", so far" << std::endl;
        }
      }
      x.delete_data();
      //if (_pos > _nr) {//FIXME do this!
      // free _prefix, _final
      //}
    }

    // construct the semigroup generated by old and coll, where this is an
    // unmodified copy of old.
    void closure (Semigroup<T>*           old,
                  const std::vector<T*>&  coll, 
                  size_t                  deg, 
                  bool                    report) {

      assert(old != this); // TODO check that old and this are equal (but not the same!!)

      std::vector<T*> irr_coll;

      // check if which of <coll> belong to <old>
      for (size_t i = 0; i < coll.size(); i++) {
        if (old->_map.find(*coll.at(i)) == old->_map.end()) { 
          irr_coll.push_back(coll.at(i));
        }
      }
      
      if (irr_coll.size() == 0) {
        return;
      }

      // reset _id in case the degree changed
      _id = static_cast<T*>(_gens.at(0)->identity());
      _found_one = false; // TODO if degree doesn't change then don't reset old
      _pos_one = 0;       // TODO if degree doesn't change then don't reset old

      _pos = 0;
      _wordlen = 0;
      _nrgens = _nrgens + irr_coll.size();
      _lenindex.clear();
      _lenindex.push_back(0);
      _index.clear();
      _index.reserve(old->_nr);

      std::vector<bool> old_new;
      old_new.reserve(old->_nr);
      for (size_t i = 0; i < old->_nr; i++) {
        old_new.push_back(false);
      }
      
      // add the old generators to new _index
      for (size_t i = 0; i < old->_lenindex.at(1); i++) {
        is_one(*_gens.at(i));
        //_index.push_back(old->_index.at(i));
        _index.push_back(i);
        old_new.at(i) = true;
      }
      // add the new generators to new _gens, elements, and _index
      for (size_t i = 0; i < irr_coll.size(); i++) {
        is_one(*irr_coll.at(i));
        _first.push_back(_gens.size());
        _final.push_back(_gens.size());
        _gens.push_back(irr_coll.at(i));
        _elements->push_back(irr_coll.at(i));
        _map.insert(std::make_pair(*irr_coll.at(i), _nr));
        _index.push_back(_nr);
        _prefix.push_back(_nr);
        _suffix.push_back(_nr);
        _genslookup.push_back(_nr);
        _nr++;
      }
      closure_expand(irr_coll.size()); // expand left/right/reduced by the number of
                                       // newly added elements
      
      bool stop = false;
      T x(_degree, _gens.at(0)); 
      // pass in sample object to, for example, pass on the semiring for
      // MatrixOverSemiring

      // process up to old->_index.at(old->_pos -1) 
      // i.e. multiply up to the last position in <old> which was multiplied
      // by all of the old generators in <old>, by all of the old and new generators
      size_t old_nr;
      while (!stop) {
        old_nr = _nr;
        while (_pos < _lenindex.at(_wordlen + 1) && !stop) {
          size_t i = _index.at(_pos); // position in _elements
          size_t b = _first.at(i);
          size_t s = _suffix.at(i); 
          if (i < old->_nr) {
            // _elements.at(i) is in old semigroup
            for (size_t j = 0; j < old->nrgens(); j++) {
              size_t k = old->_right->get(i, j);
              if (!old_new.at(k)) { // it's new!
                is_one(*_elements->at(k));
                _first.at(k) = _first.at(i);
                _final.at(k) = j;
                _prefix.at(k) = i;
                _reduced.set(i, j, true);
                _right->set(i, j, k);
                if (_wordlen != 0) {
                  _suffix.at(k) = _right->get(s, j);
                }
                _index.push_back(k);
                old_new.at(k) = true;
              } 
            }
            for (size_t j = old->nrgens(); j < nrgens(); j++) {
              closure_update(i, j, b, s, x, old, old_new);
            }
            
          } else {
            // _elements.at(i) is not in old
            for (size_t j = 0; j < nrgens(); j++) {
              closure_update(i, j, b, s, x, old, old_new);
            }
          }
          _pos++;
          //stop = (_index.at(_pos - 1) == old->_index.at(old->_pos - 1));
          stop = (_index.at(_pos - 1) == old->_pos - 1);
        } // finished words of length <wordlen> + 1
        

        if (report) {
          std::cout << "found " << _nr << " elements, ";
          std::cout << _nrrules << " rules, ";
          std::cout << "max word length " << max_word_length() << std::endl;

        }

        if (_pos > _nr || _pos == _lenindex.at(_wordlen + 1)) {
          if (_wordlen == 0) {
            for (size_t i = 0; i < _pos; i++) { 
              size_t b = _final.at(_index.at(i)); 
              for (size_t j = 0; j < _nrgens; j++) { 
                _left->set(_index.at(i), j, _right->get(_genslookup.at(j), b));
              }
            }
          } else {
            for (size_t i = _lenindex.at(_wordlen); i < _pos; i++) { 
              size_t p = _prefix.at(_index.at(i));
              size_t b = _final.at(_index.at(i)); 
              for (size_t j = 0; j < _nrgens; j++) { 
                _left->set(_index.at(i), j, _right->get(_left->get(p, j), b));
              }
            }
          }
          _wordlen++;
          closure_expand(_nr - old_nr);
          if (report) {
            std::cout << "found all words of length " << _wordlen << std::endl;
          }
        }
      }
      x.delete_data();
    }
      
  private:
    //TODO remove this 
    void inline closure_expand (size_t nr) {
      _lenindex.push_back(_index.size());
      _left->expand(nr);
      _reduced.expand(nr);
      _right->expand(nr);
    }
    void inline expand () {
      _lenindex.push_back(_nr); // words of length _wordlen + 1 start at position _nr
      _left->expand(_nr - _pos);
      _reduced.expand(_nr - _pos);
      _right->expand(_nr - _pos);
    }
    
    void closure_update (size_t i, size_t j, size_t b, size_t s, T& x, const Semigroup<T>* old, 
                         std::vector<bool>& old_new) {
      if (_wordlen != 0 && !_reduced.get(s, j)) {
        size_t r = _right->get(s, j);
        if (_found_one && r == _pos_one) {
          _right->set(i, j, _genslookup.at(b));
        } else if (r >= _lenindex.at(1)) {
          _right->set(i, j, _right->get(_left->get(_prefix.at(r), b),
                                        _final.at(r)));
        } else { 
          _right->set(i, j, _right->get(_genslookup.at(b), _final.at(r)));
        } 
      } else {
        x.redefine(_elements->at(i), _gens.at(j)); 
        auto it = _map.find(x); 
        if (it == _map.end()) { //it's new!
          is_one(x);
          _elements->push_back(static_cast<T*>(x.copy()));
          _first.push_back(b);
          _final.push_back(j);
          _map.insert(std::make_pair(*_elements->back(), _nr));
          _prefix.push_back(i);
          _reduced.set(i, j, true);
          _right->set(i, j, _nr);
          if (_wordlen == 0) { 
            _suffix.push_back(_genslookup.at(j));
          } else {
            _suffix.push_back(_right->get(s, j));
          }
          _index.push_back(_nr);
          _nr++;
        } else if (it->second < old->_nr && !old_new.at(it->second)) {
          // we didn't process it yet!
          is_one(x);
          _first.at(it->second) = b;
          _final.at(it->second) = j;
          _prefix.at(it->second) = i;
          _reduced.set(i, j, true);
          _right->set(i, j, it->second);
          if (_wordlen == 0) { 
            _suffix.push_back(_genslookup.at(j));
          } else {
            _suffix.push_back(_right->get(s, j));
          }
          _index.push_back(it->second);
          old_new.at(it->second) = true;
        } else { // it->second >= old->_nr || old_new.at(it->second)
          // it's old
          _right->set(i, j, it->second);
          _nrrules++;
        }
      }
    }

    Relation inline make_relation (size_t i, size_t j) {
      Word* lhs = this->trace(i);
      lhs->push_back(j);
      Word* rhs= this->trace(_right->get(i, j));
      return std::make_pair(lhs, rhs);
    }

    void inline is_one (T& x) {
      if (!_found_one && x == (*_id)) {
        _pos_one = _nr;
        _found_one = true;
      }
    }
    

    // TODO make as much as possible here a pointer so that they can be freed
    // when they aren't required anymore
    size_t                               _degree;
    std::vector<T*>*                     _elements;
    std::vector<size_t>                  _final;
    std::vector<size_t>                  _first;
    bool                                 _found_one;
    std::vector<T*>                      _gens;
    std::vector<size_t>                  _genslookup;  
    T*                                   _id; 
    std::vector<size_t>                  _index;
    RecVec<size_t>*                      _left;
    std::vector<size_t>                  _lenindex;
    std::unordered_map<const T, size_t>  _map;         
    size_t                               _nr;
    size_t                               _nrgens;
    size_t                               _nrrules;
    size_t                               _pos;
    size_t                               _pos_one;
    std::vector<size_t>                  _prefix;
    RecVec<bool>                         _reduced;
    RecVec<size_t>*                      _right;
    std::vector<size_t>                  _suffix;
    size_t                               _wordlen;
    //std::vector<std::vector<size_t> >    _words;
};

// TODO move to interface.cc

/*Semigroup<T>* closure_semigroup (Semigroup<T>* old, const std::vector<T*>& coll) {
  Semigroup<T>* S(new Semigroup<T>(*old));
  S.closure(coll);
  return S;
}*/

#endif