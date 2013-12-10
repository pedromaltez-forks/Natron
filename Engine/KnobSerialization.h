//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef KNOBSERIALIZATION_H
#define KNOBSERIALIZATION_H
#include <map>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/vector.hpp>

#include "Engine/Variant.h"
#include "Engine/CurveSerialization.h"


class Curve;
class KnobSerialization
{
    std::vector<Variant> _values;
    int _dimension;
    /* the keys for a specific dimension*/
    std::vector< boost::shared_ptr<Curve> > _curves;
    std::vector< std::string > _masters;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Dimension",_dimension);
        ar & boost::serialization::make_nvp("Values",_values);
        ar & boost::serialization::make_nvp("Curves",_curves);
        ar & boost::serialization::make_nvp("Masters",_masters);
    }


public:

    KnobSerialization();

    void initialize(const Knob* knob);

    const std::vector<Variant>& getValues() const { return _values; }

    int getDimension() const { return _dimension; }

    const  std::vector< boost::shared_ptr<Curve> >& getCurves() const { return _curves; }

    const std::vector< std::string > & getMasters() const { return _masters; }


};


#endif // KNOBSERIALIZATION_H
