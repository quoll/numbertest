// This file is auto-generated

#include "functions.hpp"

#include <unordered_map>

namespace Ferrum {
  std::unordered_map<std::string, FunctionID>* functionMap;

  __attribute__((constructor)) void initFunctionMap() {
    functionMap = new std::unordered_map<std::string, FunctionID>();
    std::unordered_map<std::string, FunctionID>& fnMap = *functionMap;
    fnMap["ge_abs"] = ge_abs;
    fnMap["ge_acos"] = ge_acos;
    fnMap["ge_acosh"] = ge_acosh;
    fnMap["ge_add"] = ge_add;
    fnMap["ge_asin"] = ge_asin;
    fnMap["ge_asinh"] = ge_asinh;
    fnMap["ge_atan"] = ge_atan;
    fnMap["ge_atan2"] = ge_atan2;
    fnMap["ge_atanh"] = ge_atanh;
    fnMap["ge_cbrt"] = ge_cbrt;
    fnMap["ge_cdf_norm"] = ge_cdf_norm;
    fnMap["ge_cdf_norm_inv"] = ge_cdf_norm_inv;
    fnMap["ge_ceil"] = ge_ceil;
    fnMap["ge_copysign"] = ge_copysign;
    fnMap["ge_cos"] = ge_cos;
    fnMap["ge_cosh"] = ge_cosh;
    fnMap["ge_div"] = ge_div;
    fnMap["ge_elu"] = ge_elu;
    fnMap["ge_erf"] = ge_erf;
    fnMap["ge_erf_inv"] = ge_erf_inv;
    fnMap["ge_erfc"] = ge_erfc;
    fnMap["ge_erfcinv"] = ge_erfcinv;
    fnMap["ge_exp"] = ge_exp;
    fnMap["ge_exp10"] = ge_exp10;
    fnMap["ge_exp2"] = ge_exp2;
    fnMap["ge_expm1"] = ge_expm1;
    fnMap["ge_floor"] = ge_floor;
    fnMap["ge_fmax"] = ge_fmax;
    fnMap["ge_fmin"] = ge_fmin;
    fnMap["ge_fmod"] = ge_fmod;
    fnMap["ge_frac"] = ge_frac;
    fnMap["ge_frem"] = ge_frem;
    fnMap["ge_gamma"] = ge_gamma;
    fnMap["ge_hypot"] = ge_hypot;
    fnMap["ge_inv"] = ge_inv;
    fnMap["ge_inv_cbrt"] = ge_inv_cbrt;
    fnMap["ge_inv_sqrt"] = ge_inv_sqrt;
    fnMap["ge_lgamma"] = ge_lgamma;
    fnMap["ge_linear_frac"] = ge_linear_frac;
    fnMap["ge_log"] = ge_log;
    fnMap["ge_log10"] = ge_log10;
    fnMap["ge_log1p"] = ge_log1p;
    fnMap["ge_log2"] = ge_log2;
    fnMap["ge_modf"] = ge_modf;
    fnMap["ge_mul"] = ge_mul;
    fnMap["ge_pow"] = ge_pow;
    fnMap["ge_pow2o3"] = ge_pow2o3;
    fnMap["ge_pow3o2"] = ge_pow3o2;
    fnMap["ge_powx"] = ge_powx;
    fnMap["ge_ramp"] = ge_ramp;
    fnMap["ge_relu"] = ge_relu;
    fnMap["ge_round"] = ge_round;
    fnMap["ge_scale_shift"] = ge_scale_shift;
    fnMap["ge_sigmoid"] = ge_sigmoid;
    fnMap["ge_sin"] = ge_sin;
    fnMap["ge_sincos"] = ge_sincos;
    fnMap["ge_sinh"] = ge_sinh;
    fnMap["ge_sqr"] = ge_sqr;
    fnMap["ge_sqrt"] = ge_sqrt;
    fnMap["ge_sub"] = ge_sub;
    fnMap["ge_tan"] = ge_tan;
    fnMap["ge_tanh"] = ge_tanh;
    fnMap["ge_trunc"] = ge_trunc;
    fnMap["uplo_abs"] = uplo_abs;
    fnMap["uplo_acos"] = uplo_acos;
    fnMap["uplo_acosh"] = uplo_acosh;
    fnMap["uplo_add"] = uplo_add;
    fnMap["uplo_asin"] = uplo_asin;
    fnMap["uplo_asinh"] = uplo_asinh;
    fnMap["uplo_atan"] = uplo_atan;
    fnMap["uplo_atan2"] = uplo_atan2;
    fnMap["uplo_atanh"] = uplo_atanh;
    fnMap["uplo_cbrt"] = uplo_cbrt;
    fnMap["uplo_cdf_norm"] = uplo_cdf_norm;
    fnMap["uplo_cdf_norm_inv"] = uplo_cdf_norm_inv;
    fnMap["uplo_ceil"] = uplo_ceil;
    fnMap["uplo_copysign"] = uplo_copysign;
    fnMap["uplo_cos"] = uplo_cos;
    fnMap["uplo_cosh"] = uplo_cosh;
    fnMap["uplo_div"] = uplo_div;
    fnMap["uplo_elu"] = uplo_elu;
    fnMap["uplo_erf"] = uplo_erf;
    fnMap["uplo_erf_inv"] = uplo_erf_inv;
    fnMap["uplo_erfc"] = uplo_erfc;
    fnMap["uplo_erfc_inv"] = uplo_erfc_inv;
    fnMap["uplo_exp"] = uplo_exp;
    fnMap["uplo_exp10"] = uplo_exp10;
    fnMap["uplo_exp2"] = uplo_exp2;
    fnMap["uplo_expm1"] = uplo_expm1;
    fnMap["uplo_floor"] = uplo_floor;
    fnMap["uplo_fmax"] = uplo_fmax;
    fnMap["uplo_fmin"] = uplo_fmin;
    fnMap["uplo_fmod"] = uplo_fmod;
    fnMap["uplo_frac"] = uplo_frac;
    fnMap["uplo_frem"] = uplo_frem;
    fnMap["uplo_gamma"] = uplo_gamma;
    fnMap["uplo_hypot"] = uplo_hypot;
    fnMap["uplo_inv"] = uplo_inv;
    fnMap["uplo_inv_cbrt"] = uplo_inv_cbrt;
    fnMap["uplo_inv_sqrt"] = uplo_inv_sqrt;
    fnMap["uplo_lgamma"] = uplo_lgamma;
    fnMap["uplo_linear_frac"] = uplo_linear_frac;
    fnMap["uplo_log"] = uplo_log;
    fnMap["uplo_log10"] = uplo_log10;
    fnMap["uplo_log1p"] = uplo_log1p;
    fnMap["uplo_log2"] = uplo_log2;
    fnMap["uplo_modf"] = uplo_modf;
    fnMap["uplo_mul"] = uplo_mul;
    fnMap["uplo_pow"] = uplo_pow;
    fnMap["uplo_pow2o3"] = uplo_pow2o3;
    fnMap["uplo_pow3o2"] = uplo_pow3o2;
    fnMap["uplo_powx"] = uplo_powx;
    fnMap["uplo_ramp"] = uplo_ramp;
    fnMap["uplo_relu"] = uplo_relu;
    fnMap["uplo_round"] = uplo_round;
    fnMap["uplo_scale_shift"] = uplo_scale_shift;
    fnMap["uplo_sigmoid"] = uplo_sigmoid;
    fnMap["uplo_sin"] = uplo_sin;
    fnMap["uplo_sincos"] = uplo_sincos;
    fnMap["uplo_sinh"] = uplo_sinh;
    fnMap["uplo_sqr"] = uplo_sqr;
    fnMap["uplo_sqrt"] = uplo_sqrt;
    fnMap["uplo_sub"] = uplo_sub;
    fnMap["uplo_tan"] = uplo_tan;
    fnMap["uplo_tanh"] = uplo_tanh;
    fnMap["uplo_trunc"] = uplo_trunc;
    fnMap["vector_abs"] = vector_abs;
    fnMap["vector_acos"] = vector_acos;
    fnMap["vector_acosh"] = vector_acosh;
    fnMap["vector_add"] = vector_add;
    fnMap["vector_asin"] = vector_asin;
    fnMap["vector_asinh"] = vector_asinh;
    fnMap["vector_atan"] = vector_atan;
    fnMap["vector_atan2"] = vector_atan2;
    fnMap["vector_atanh"] = vector_atanh;
    fnMap["vector_cbrt"] = vector_cbrt;
    fnMap["vector_cdf_norm"] = vector_cdf_norm;
    fnMap["vector_cdf_norm_inv"] = vector_cdf_norm_inv;
    fnMap["vector_ceil"] = vector_ceil;
    fnMap["vector_copy"] = vector_copy;
    fnMap["vector_copysign"] = vector_copysign;
    fnMap["vector_cos"] = vector_cos;
    fnMap["vector_cosh"] = vector_cosh;
    fnMap["vector_div"] = vector_div;
    fnMap["vector_elu"] = vector_elu;
    fnMap["vector_equals"] = vector_equals;
    fnMap["vector_erf"] = vector_erf;
    fnMap["vector_erf_inv"] = vector_erf_inv;
    fnMap["vector_erfc"] = vector_erfc;
    fnMap["vector_erfc_inv"] = vector_erfc_inv;
    fnMap["vector_exp"] = vector_exp;
    fnMap["vector_exp10"] = vector_exp10;
    fnMap["vector_exp2"] = vector_exp2;
    fnMap["vector_expm1"] = vector_expm1;
    fnMap["vector_floor"] = vector_floor;
    fnMap["vector_fmax"] = vector_fmax;
    fnMap["vector_fmin"] = vector_fmin;
    fnMap["vector_fmod"] = vector_fmod;
    fnMap["vector_frac"] = vector_frac;
    fnMap["vector_frem"] = vector_frem;
    fnMap["vector_gamma"] = vector_gamma;
    fnMap["vector_hypot"] = vector_hypot;
    fnMap["vector_inv"] = vector_inv;
    fnMap["vector_inv_cbrt"] = vector_inv_cbrt;
    fnMap["vector_inv_sqrt"] = vector_inv_sqrt;
    fnMap["vector_lgamma"] = vector_lgamma;
    fnMap["vector_linear_frac"] = vector_linear_frac;
    fnMap["vector_log"] = vector_log;
    fnMap["vector_log10"] = vector_log10;
    fnMap["vector_log1p"] = vector_log1p;
    fnMap["vector_log2"] = vector_log2;
    fnMap["vector_modf"] = vector_modf;
    fnMap["vector_mul"] = vector_mul;
    fnMap["vector_pow"] = vector_pow;
    fnMap["vector_pow2o3"] = vector_pow2o3;
    fnMap["vector_pow3o2"] = vector_pow3o2;
    fnMap["vector_powx"] = vector_powx;
    fnMap["vector_ramp"] = vector_ramp;
    fnMap["vector_relu"] = vector_relu;
    fnMap["vector_round"] = vector_round;
    fnMap["vector_scale_shift"] = vector_scale_shift;
    fnMap["vector_set"] = vector_set;
    fnMap["vector_sigmoid"] = vector_sigmoid;
    fnMap["vector_sin"] = vector_sin;
    fnMap["vector_sincos"] = vector_sincos;
    fnMap["vector_sinh"] = vector_sinh;
    fnMap["vector_sqr"] = vector_sqr;
    fnMap["vector_sqrt"] = vector_sqrt;
    fnMap["vector_sub"] = vector_sub;
    fnMap["vector_swap"] = vector_swap;
    fnMap["vector_tan"] = vector_tan;
    fnMap["vector_tanh"] = vector_tanh;
    fnMap["vector_trunc"] = vector_trunc;
  }

  __attribute__((destructor)) void cleanupFunctionMap() {
    delete functionMap;
    functionMap = nullptr;
  }

} // namespace Ferrum

