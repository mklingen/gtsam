/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    matlab.h
 * @brief   Contains *generic* global functions designed particularly for the matlab interface
 * @author  Stephen Williams
 */

#pragma once

#include <gtsam/slam/ProjectionFactor.h>
#include <gtsam/nonlinear/NonlinearFactorGraph.h>
#include <gtsam/nonlinear/NonlinearFactor.h>
#include <gtsam/nonlinear/Values.h>
#include <gtsam/geometry/Point2.h>
#include <gtsam/geometry/Point3.h>
#include <gtsam/geometry/Pose2.h>
#include <gtsam/geometry/Pose3.h>
#include <gtsam/geometry/Cal3_S2.h>
#include <gtsam/geometry/SimpleCamera.h>

#include <boost/foreach.hpp>

#include <exception>

namespace gtsam {

namespace utilities {

// Create a KeyList from indices
FastList<Key> createKeyList(const Vector& I) {
  FastList<Key> set;
  for (int i = 0; i < I.size(); i++)
    set.push_back(I[i]);
  return set;
}

// Create a KeyList from indices using symbol
FastList<Key> createKeyList(string s, const Vector& I) {
  FastList<Key> set;
  char c = s[0];
  for (int i = 0; i < I.size(); i++)
    set.push_back(Symbol(c, I[i]));
  return set;
}

// Create a KeyVector from indices
FastVector<Key> createKeyVector(const Vector& I) {
  FastVector<Key> set;
  for (int i = 0; i < I.size(); i++)
    set.push_back(I[i]);
  return set;
}

// Create a KeyVector from indices using symbol
FastVector<Key> createKeyVector(string s, const Vector& I) {
  FastVector<Key> set;
  char c = s[0];
  for (int i = 0; i < I.size(); i++)
    set.push_back(Symbol(c, I[i]));
  return set;
}

// Create a KeySet from indices
FastSet<Key> createKeySet(const Vector& I) {
  FastSet<Key> set;
  for (int i = 0; i < I.size(); i++)
    set.insert(I[i]);
  return set;
}

// Create a KeySet from indices using symbol
FastSet<Key> createKeySet(string s, const Vector& I) {
  FastSet<Key> set;
  char c = s[0];
  for (int i = 0; i < I.size(); i++)
    set.insert(symbol(c, I[i]));
  return set;
}

/// Extract all Point2 values into a single matrix [x y]
Matrix extractPoint2(const Values& values) {
  size_t j = 0;
  Values::ConstFiltered<Point2> points = values.filter<Point2>();
  Matrix result(points.size(), 2);
  BOOST_FOREACH(const Values::ConstFiltered<Point2>::KeyValuePair& key_value, points)
    result.row(j++) = key_value.value.vector();
  return result;
}

/// Extract all Point3 values into a single matrix [x y z]
Matrix extractPoint3(const Values& values) {
  Values::ConstFiltered<Point3> points = values.filter<Point3>();
  Matrix result(points.size(), 3);
  size_t j = 0;
  BOOST_FOREACH(const Values::ConstFiltered<Point3>::KeyValuePair& key_value, points)
    result.row(j++) = key_value.value.vector();
  return result;
}

/// Extract all Pose2 values into a single matrix [x y theta]
Matrix extractPose2(const Values& values) {
  Values::ConstFiltered<Pose2> poses = values.filter<Pose2>();
  Matrix result(poses.size(), 3);
  size_t j = 0;
  BOOST_FOREACH(const Values::ConstFiltered<Pose2>::KeyValuePair& key_value, poses)
    result.row(j++) << key_value.value.x(), key_value.value.y(), key_value.value.theta();
  return result;
}

/// Extract all Pose3 values
Values allPose3s(const Values& values) {
  return values.filter<Pose3>();
}

/// Extract all Pose3 values into a single matrix [r11 r12 r13 r21 r22 r23 r31 r32 r33 x y z]
Matrix extractPose3(const Values& values) {
  Values::ConstFiltered<Pose3> poses = values.filter<Pose3>();
  Matrix result(poses.size(), 12);
  size_t j = 0;
  BOOST_FOREACH(const Values::ConstFiltered<Pose3>::KeyValuePair& key_value, poses) {
    result.row(j).segment(0, 3) << key_value.value.rotation().matrix().row(0);
    result.row(j).segment(3, 3) << key_value.value.rotation().matrix().row(1);
    result.row(j).segment(6, 3) << key_value.value.rotation().matrix().row(2);
    result.row(j).tail(3) = key_value.value.translation().vector();
    j++;
  }
  return result;
}

/// Perturb all Point2 values using normally distributed noise
void perturbPoint2(Values& values, double sigma, int32_t seed = 42u) {
  noiseModel::Isotropic::shared_ptr model = noiseModel::Isotropic::Sigma(2,
      sigma);
  Sampler sampler(model, seed);
  BOOST_FOREACH(const Values::ConstFiltered<Point2>::KeyValuePair& key_value, values.filter<Point2>()) {
    values.update(key_value.key, key_value.value.retract(sampler.sample()));
  }
}

/// Perturb all Pose2 values using normally distributed noise
void perturbPose2(Values& values, double sigmaT, double sigmaR, int32_t seed =
    42u) {
  noiseModel::Diagonal::shared_ptr model = noiseModel::Diagonal::Sigmas(
      Vector3(sigmaT, sigmaT, sigmaR));
  Sampler sampler(model, seed);
  BOOST_FOREACH(const Values::ConstFiltered<Pose2>::KeyValuePair& key_value, values.filter<Pose2>()) {
    values.update(key_value.key, key_value.value.retract(sampler.sample()));
  }
}

/// Perturb all Point3 values using normally distributed noise
void perturbPoint3(Values& values, double sigma, int32_t seed = 42u) {
  noiseModel::Isotropic::shared_ptr model = noiseModel::Isotropic::Sigma(3,
      sigma);
  Sampler sampler(model, seed);
  BOOST_FOREACH(const Values::ConstFiltered<Point3>::KeyValuePair& key_value, values.filter<Point3>()) {
    values.update(key_value.key, key_value.value.retract(sampler.sample()));
  }
}

/// Insert a number of initial point values by backprojecting
void insertBackprojections(Values& values, const SimpleCamera& camera,
    const Vector& J, const Matrix& Z, double depth) {
  if (Z.rows() != 2)
    throw std::invalid_argument("insertBackProjections: Z must be 2*K");
  if (Z.cols() != J.size())
    throw std::invalid_argument(
        "insertBackProjections: J and Z must have same number of entries");
  for (int k = 0; k < Z.cols(); k++) {
    Point2 p(Z(0, k), Z(1, k));
    Point3 P = camera.backproject(p, depth);
    values.insert(J(k), P);
  }
}

/// Insert multiple projection factors for a single pose key
void insertProjectionFactors(NonlinearFactorGraph& graph, Key i,
    const Vector& J, const Matrix& Z, const SharedNoiseModel& model,
    const Cal3_S2::shared_ptr K, const Pose3& body_P_sensor = Pose3()) {
  if (Z.rows() != 2)
    throw std::invalid_argument("addMeasurements: Z must be 2*K");
  if (Z.cols() != J.size())
    throw std::invalid_argument(
        "addMeasurements: J and Z must have same number of entries");
  for (int k = 0; k < Z.cols(); k++) {
    graph.push_back(
        boost::make_shared<GenericProjectionFactor<Pose3, Point3> >(
            Point2(Z(0, k), Z(1, k)), model, i, Key(J(k)), K, body_P_sensor));
  }
}

/// Calculate the errors of all projection factors in a graph
Matrix reprojectionErrors(const NonlinearFactorGraph& graph,
    const Values& values) {
  // first count
  size_t K = 0, k = 0;
  BOOST_FOREACH(const NonlinearFactor::shared_ptr& f, graph)
    if (boost::dynamic_pointer_cast<const GenericProjectionFactor<Pose3, Point3> >(
        f))
      ++K;
  // now fill
  Matrix errors(2, K);
  BOOST_FOREACH(const NonlinearFactor::shared_ptr& f, graph) {
    boost::shared_ptr<const GenericProjectionFactor<Pose3, Point3> > p =
        boost::dynamic_pointer_cast<const GenericProjectionFactor<Pose3, Point3> >(
            f);
    if (p)
      errors.col(k++) = p->unwhitenedError(values);
  }
  return errors;
}

/// Convert from local to world coordinates
Values localToWorld(const Values& local, const Pose2& base,
    const FastVector<Key> user_keys = FastVector<Key>()) {

  Values world;

  // if no keys given, get all keys from local values
  FastVector<Key> keys(user_keys);
  if (keys.size()==0)
    keys = FastVector<Key>(local.keys());

  // Loop over all keys
  BOOST_FOREACH(Key key, keys) {
    try {
      // if value is a Pose2, compose it with base pose
      Pose2 pose = local.at<Pose2>(key);
      world.insert(key, base.compose(pose));
    } catch (std::exception e1) {
      try {
        // if value is a Point2, transform it from base pose
        Point2 point = local.at<Point2>(key);
        world.insert(key, base.transform_from(point));
      } catch (std::exception e2) {
        // if not Pose2 or Point2, do nothing
      }
    }
  }
  return world;
}

}
}

