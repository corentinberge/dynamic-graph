// Copyright 2010 Thomas Moulard.
//

#include <string>

#include <boost/foreach.hpp>
#include <boost/format.hpp>

#include <Eigen/Dense>

#include <dynamic-graph/debug.h>
#include <dynamic-graph/entity.h>
#include <dynamic-graph/factory.h>
#include <dynamic-graph/pool.h>
#include <dynamic-graph/eigen-io.h>
#include <dynamic-graph/linear-algebra.h>
#include <dynamic-graph/signal-caster.h>
#include <dynamic-graph/signal.h>
#include <dynamic-graph/signal-cast-helper.h>

#include "signal-cast-registerer-libA.hh"
#include "signal-cast-registerer-libB.hh"

#define BOOST_TEST_MODULE signal_cast_registerer

#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
#include <iostream>

using boost::test_tools::output_test_stream;


typedef Eigen::VectorXd Vector;
typedef Eigen::MatrixXd Matrix;


struct EigenCastRegisterer_V : public dynamicgraph::SignalCastRegisterer
{
  typedef Vector bnuVector;

  EigenCastRegisterer_V () :
    SignalCastRegisterer
    (typeid(bnuVector), dispVector, castVector, traceVector)
  {}

  static boost::any castVector (std::istringstream& iss)
  {
    bnuVector res;
    iss >> res;
    return res;
  }

  static void dispVector (const boost::any& object, std::ostream& os)
  {
    const bnuVector& v = boost::any_cast<bnuVector> (object);
    os << "[ ";
    for (int i = 0; i < v.size (); ++i)
      os << v(i) << " ";
    os << " ];" << std::endl;
  }

  static void traceVector (const boost::any& object, std::ostream& os)
  {
    const bnuVector& v = boost::any_cast<bnuVector> (object);
    for (int i = 0; i < v.size (); ++i)
      os << v(i) << " ";
    os << std::endl;
  }
};

template<typename Derived>
struct EigenCastRegisterer_M : public dynamicgraph::SignalCastRegisterer
{
    typedef Matrix bnuMatrix;

    EigenCastRegisterer_M () :
        SignalCastRegisterer
        (typeid(bnuMatrix), dispMatrix, castMatrix, traceMatrix)
    {}

    static boost::any castMatrix (std::istringstream& iss)
    {
        bnuMatrix res;
        iss >> res;
        return res;
    }

    static void dispMatrix (const boost::any& object, std::ostream& os)
    {
        const bnuMatrix& m = boost::any_cast<bnuMatrix> (object);
        os << "[ ";
        for (int i = 0; i < m.rows(); ++i){
            os << "[ ";
            for (int j = 0; j < m.cols(); ++j) {
                os << m(i, j) << " ";
            }
            if (i != m.rows()-1){
                os << "]; ";
            }
            else{
                os << "] ";
            }
        }
        os << " ];" << std::endl;
    }

    static void traceMatrix (const boost::any& object, std::ostream& os)
    {
        const bnuMatrix& m = boost::any_cast<bnuMatrix> (object);
        for (int i = 0; i < m.rows(); ++i){
            for (int j = 0; j < m.cols(); ++j){
                os << m(i,j) << " ";
            }
        }
        os << std::endl;
    }
};

EigenCastRegisterer_V myVectorCast;
EigenCastRegisterer_M<int> myMatrixCast;

// Define a new cast with a type that supports streaming operators to
// and from it (this could be automated with macros).
dynamicgraph::DefaultCastRegisterer<bool> myBooleanCast;


// Check standard double cast registerer.
BOOST_AUTO_TEST_CASE (standard_double_registerer)
{
  dynamicgraph::Signal<double, int> mySignal("out");

  typedef std::pair<std::string, std::string> test_t;
  std::vector<test_t> values;

  values.push_back (std::make_pair ("42.0", "42\n"));
  values.push_back (std::make_pair ("42.5", "42.5\n"));
  values.push_back (std::make_pair ("-12.", "-12\n"));

  // Double special values.
  // FIXME: these tests are failing :(
  values.push_back (std::make_pair ("inf", "inf\n"));
  values.push_back (std::make_pair ("-inf", "-inf\n"));
  values.push_back (std::make_pair ("nan", "nan\n"));

  BOOST_FOREACH(const test_t& test, values)
    {
      // Set
      std::istringstream value (test.first);
      mySignal.set (value);

      // Get
      {
	output_test_stream output;
	mySignal.get (output);
	BOOST_CHECK (output.is_equal (test.second));
      }

      // Trace
      {
	output_test_stream output;
	mySignal.trace (output);
	BOOST_CHECK (output.is_equal (test.second));

      }
    }

  // Check invalid values.
  // FIXME: this test is failing for now.
  std::istringstream value ("This is not a valid double.");
  BOOST_CHECK_THROW (mySignal.set (value), std::exception);
}


// Check a custom cast registerer for Boost uBLAS vectors.
BOOST_AUTO_TEST_CASE (custom_vector_registerer)
{

  dynamicgraph::Signal<dynamicgraph::Vector, int> myVectorSignal("vector");

  // Print the signal name.
  {
    output_test_stream output;
    output << myVectorSignal;
    BOOST_CHECK (output.is_equal ("Sig:vector (Type Cst)"));
  }

  for (unsigned int i = 0; i < 5; ++i)
    {
      Vector  v = Vector::Unit(5,i) ;
      std::ostringstream os;
      os << v;
      std::istringstream ss ("[5]("+os.str ()+")");

      // Set signal value.
      myVectorSignal.set (ss);

      // Print out signal value.
      output_test_stream output;
      myVectorSignal.get (output);

      boost::format fmt ("[ %d %d %d %d %d  ];\n");
      fmt
	% (i == 0)
	% (i == 1)
	% (i == 2)
	% (i == 3)
	% (i == 4);

      BOOST_CHECK (output.is_equal (fmt.str ()));
    }

  //Catch Exception of ss (not good input)

  //ss[0] != "["
  try {
      std::istringstream ss ("test");
      myVectorSignal.set (ss);
  } catch(ExceptionSignal e) {
      std::cout << "Test passed : ss[0] != \"[\"";
  }

  //ss[1] != %i
    try {
        std::istringstream ss ("[test");
        myVectorSignal.set (ss);
    } catch(ExceptionSignal e) {
        std::cout << "Test passed : ss[1] != %i";
    }

  //ss[2] != "]"
    try {
        std::istringstream ss ("[5[");
        myVectorSignal.set (ss);
    } catch(ExceptionSignal e) {
        std::cout << "Test passed : ss[2] != \"]\"";
    }

    //ss[3] != "("
    try {
        std::istringstream ss ("[5]test");
        myVectorSignal.set (ss);
    } catch(ExceptionSignal e) {
        std::cout << "Test passed : ss[3] != \"(\"";
    }

    //ss[4] != ' ' || ','
    try {
        std::istringstream ss ("[5](1, ");
        myVectorSignal.set (ss);
    } catch(ExceptionSignal e) {
        BOOST_ERROR("Can't happened");
    }

    //ss[-1] != ")"
    try {
        std::istringstream ss ("[5](1,2,3,4,5]");
        myVectorSignal.set (ss);
    } catch(ExceptionSignal e) {
        std::cout << "Test passed : ss[-1] != \")\"";
    }
}

BOOST_AUTO_TEST_CASE (custom_matrix_registerer) {

    dynamicgraph::Signal<dynamicgraph::Matrix, int> myMatrixSignal("matrix");

    // Print the signal name.
    {
        output_test_stream output;
        output << myMatrixSignal;
        BOOST_CHECK (output.is_equal ("Sig:matrix (Type Cst)"));
    }

    //Catch Exception of ss (not good input)

    //ss[0] != "["
    try {
        std::istringstream ss("test");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << "Test passed : ss[0] != \"[\"";
    }

    //ss[1] != %i
    try {
        std::istringstream ss("[test");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << "Test passed : ss[1] != %i";
    }

    //ss[2] != ","
    try {
        std::istringstream ss("[5[");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << "Test passed : ss[2] != \",\"";
    }

    //ss[3] != %i
    try {
        std::istringstream ss("[5,c");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << "Test passed : ss[3] != %i";
    }

    //ss[4] != "]"
    try {
        std::istringstream ss("[5,3[");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << "Test passed : ss[4] != \"]\"";
    }

    //ss[5] != "("
    try {
        std::istringstream ss("[5,3]test");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << "Test passed : ss[5] != \"(\"";
    }

    //ss[6] != "("
    try {
        std::istringstream ss("[5,3](test");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << "Test passed : ss[6] != \"(\"";
    }

    //ss[8] != " " || ","
    try {
        std::istringstream ss("[5,3]((1,");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        BOOST_ERROR("Can't happened");
    }

    //ss[6+n] != ")"
    try {
        std::istringstream ss("[5,3]((1,2,3]");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << ("ss[6+n] != \")\"");
    }

    //ss[-3] != ")"
    try {
        std::istringstream ss("[5,1]((1)(2)(3[");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << "Test passed : ss[5] != \")\"";
    }

    //ss[-3] != ")"
    try {
        std::istringstream ss("[5,1]((1)(2)(3)[");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << "Test passed : ss[5] != \")\"";
    }

    //ss[-1]!= ")"
    try {
        std::istringstream ss("[3,1]((1)(2),(3)[");
        myMatrixSignal.set(ss);
    } catch (ExceptionSignal e) {
        std::cout << "Test passed : ss[5] != \")\" and ignore \",\"";
    }

    //[...]((...))
}

// One issue with the strategy used by the
// dynamicgraph::SignalCastRegisterer is that it relies on the
// typeid. In practice, it means that two signals defined in two
// different libraries will have different typeid and one will not be
// able to plug one into the other unless the symbol have merged when
// the plug-in is loaded. See man(3) dlopen in Linux for more
// information about plug-in loading and the RTLD_GLOBAL flag
// necessary to make cast registerer work as expected.
//
// Here we make sure that two instances of the same type
// declared in two separate libraries are resolved into the
// same typeid.
/*BOOST_AUTO_TEST_CASE (typeid_issue)
{
  BOOST_CHECK (typeid(vA) == typeid(vB));
  BOOST_CHECK_EQUAL (std::string (typeid(vA).name ()),
		     std::string (typeid(vB).name ()));
}*/