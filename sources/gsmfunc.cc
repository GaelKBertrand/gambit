//#
//# FILE: gsmfunc.cc -- handles initialization of defined functions for GSM
//#                     companion to GSM
//#
//# $Id$
//#



// Usage:
//
// For each module of functions to be added to GSM, create an 
// 'void Init_<name>( void )' function that calls GSM::AddFunction()
// to add the desired functions into GSM, where <name> is the name of the 
// particular module.  Then create a header file for that module, and include
// it in this file.  Put the Init_<name>() function call in the 
// GSM::InitFunctions() in this file.
//
// The definition for GSM::AddFunction is:
//   GSM::AddFunction( gString reference, FuncDescObj *func_desc_obj );
//
// FuncDescObj is a class which stores the characteristics of each
// particular function, such as the number of parameters and the type of
// each parameter.  GSM will pass the function parameters in the form of
// an array of pointers to Portion class, so all parameter types should
// be decendents of Portion class.  Each function is expected to have
// a return type of 'Portion *'.
//
// The FuncDescObj is created by passing its constructor with a function
// pointer and the number of parameters.  Then the type of each parameter
// can be specified with FuncDescObj::ParamType() function, as shown in
// the example.  It is important to define the types for all parameters.
// The valid parameter types are listed in portion.h, typedef PortionType.
//
// Example:
//   void Init_myfunc( void )
//   {
//     FuncDescObj *FuncObj;
//
//     FuncObj = new FuncDescObj( GCL_Sqr, 1 );
//     FuncObj->SetParamInfo( 0, "n", porNUMERICAL, NO_DEFAULT_VALUE );
//     GSM::AddFunction( (gString) "Sqr",  FuncObj );
//
//     FuncObj = new FuncDescObj( GCL_Angle, 2 );
//     FuncObj->SetParamInfo( 0, "x", porDOUBLE, new numerical_Portion<double>( 1 ) );
//     FuncObj->SetParamInfo( 1, "y", porDOUBLE, new numerical_Portion<double>( 1 ) );
//     GSM::AddFunction( (gString) "Angle",  FuncObj );
//   }
//
// Specifying a parameter type as porNUMERICAL means that the parameter
// can be any of porDOUBLE, porINTEGER, or porRATIONAL types.  It is then
// up to the function implementation to explicitly type cast those parameters.
// Specifying a parameter type as porNUMERICAL also means that that parameter
// cannot have a default value associated with it.
//
// If a function returns 0 (null pointer), GSM will halt and give a general
// error message.  So it may be a good idea to initialize the return value
// to 0 until all necessary processing has been completed.
//
// All the header files should #include "gsm.h".  The definitions for 
// Portion types are already included in "gsm.h".
//
// Every function is expected to delete all of its pointer parameters, except
// in cases when one of the pointers is used as the return value.
//
// Take a look at gclmath.h and gclmath.cc to see how it works.


#include <assert.h>
#include "gsmfunc.h"



// The header files for each module should be placed here:

#include "gclmath.h"



// This function is called once at the first instance of GSM.
// The Init function of each module should be placed in this function:

void GSM::InitFunctions( void )
{
  Init_gclmath( this );
}





//------------------------------------------------------------------
  // Function descriptor objects
//------------------------------------------------------------------


FuncDescObj::FuncDescObj( Portion* (*funcname)(Portion**), const int size )
     :function(funcname),num_of_params( size )
{
  assert( size >= 0 );

  ParamInfo = new ParamInfoType[ num_of_params ];
}


FuncDescObj::~FuncDescObj()
{
  int i;
  for( i = 0; i < num_of_params; i++ )
  {
    delete ParamInfo[ i ].DefaultValue;
  }
  delete[] ParamInfo;
}


Portion* FuncDescObj::CallFunction( Portion** param )
{
  return function( param );
}


int FuncDescObj::NumParams( void ) const
{
  return num_of_params;
}


gString FuncDescObj::ParamName( const int index ) const
{
  assert( index >= 0 && index < num_of_params );

  return ParamInfo[ index ].Name;
}


PortionType FuncDescObj::ParamType( const int index ) const
{
  assert( index >= 0 && index < num_of_params );

  return ParamInfo[ index ].Type;
}


Portion* FuncDescObj::ParamDefaultValue( const int index ) const
{
  assert( index >= 0 && index < num_of_params );

  if( ParamInfo[ index ].DefaultValue == NO_DEFAULT_VALUE )
  {
    return 0;
  }
  else
  {
    return ParamInfo[ index ].DefaultValue->Copy();
  }
}



int FuncDescObj::FindParamName( const gString& name ) const
{
  int i;
  int result = -1;

  for( i = 0; i < num_of_params; i++ )
  {
    if( ParamInfo[ i ].Name == name )
    {
      result = i;
      break;
    }
  }
  return result;
}


void FuncDescObj::SetParamInfo
  ( 
   const int          index, 
   const gString&     name, 
   const PortionType  type, 
   Portion*           default_value
   )
{
  int i;
  int repeated_variable_declaration = false;

  assert( index >= 0 && index < num_of_params );

  for( i = 0; i < num_of_params; i++ )
  {
    if( ParamInfo[ i ].Name == name )
    {
      repeated_variable_declaration = true;
      break;
    }
  }

#ifndef NDEBUG
  if( repeated_variable_declaration )
  {
    gerr << "FuncDescObj Error: multiple parameters of a functions were\n";
    gerr << "                   declared with the same formal name \"";
    gerr << name << "\"\n";
    gerr << "                   during initialization\n";
  }
  assert( !repeated_variable_declaration );
#endif // NDEBUG
  
  ParamInfo[ index ].Type = type;
  ParamInfo[ index ].Name = name;
  ParamInfo[ index ].DefaultValue = default_value;
}



//-------------------------------------------------------------------
//                      CallFunctionObject
//-------------------------------------------------------------------


  
CallFunctionObject::CallFunctionObject( const gString& name, 
				        FuncDescObj* func )
     :func_name( name ), func_desc_obj( func )
{
  int i;

  param = new Portion* [ func_desc_obj->NumParams() ];
  current_param_index = 0;
  
  for( i = 0; i < func_desc_obj->NumParams(); i++ )
  {
    param[ i ] = func_desc_obj->ParamDefaultValue( i );
  }
}


PortionType CallFunctionObject::GetCurrParamType( void ) const
{
  if( current_param_index < func_desc_obj->NumParams() )
  {
    return func_desc_obj->ParamType( current_param_index );
  }
  else // ( current_param_index >= func_desc_obj->NumParams() )
  {
    gerr << "CallFunctionObject Error: too many parameters specified for\n";
    gerr << "                          function \"" << func_name << "\"\n";
    return porERROR;
  }
}


void CallFunctionObject::SetCurrParam( Portion *new_param )
{
  if( current_param_index < func_desc_obj->NumParams() )
  {
    if( param[ current_param_index ] != 0 )
    {
      delete param[ current_param_index ];
    }
    param[ current_param_index ] = new_param;
    current_param_index++;
  }
  else // ( current_param_index >= func_desc_obj->NumParams() )
  {
    gerr << "CallFunctionObject Error: too many parameters specified for\n";
    gerr << "                          function \"" << func_name << "\"\n";
  }
}


void CallFunctionObject::SetCurrParamIndex( const int index )
{
  current_param_index = index;
}


int CallFunctionObject::GetCurrParamIndex( void ) const
{
  return current_param_index;
}


int CallFunctionObject::FindParamName( const gString& name ) const
{
  return func_desc_obj->FindParamName( name );
}


gString CallFunctionObject::FuncName( void ) const
{
  return func_name;
}


Portion* CallFunctionObject::CallFunction( void )
{
  int i;

  for( i = 0; i < func_desc_obj->NumParams(); i++ )
  {
    if( param[ i ] == 0 )
    {
      gerr << "GSM Error: required parameter \"" << func_desc_obj->ParamName( i ) << "\"" << " not found while executing\n";
      gerr << "           CallFunction() on function \"" << func_name << "\"\n";
      return 0;
    }
    assert( param[ i ] != 0 );
  }
  return func_desc_obj->CallFunction( param );
}


