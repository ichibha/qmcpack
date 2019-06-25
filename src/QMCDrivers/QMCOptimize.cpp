//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Miguel Morales, moralessilva2@llnl.gov, Lawrence Livermore National Laboratory
//                    Ken Esler, kpesler@gmail.com, University of Illinois at Urbana-Champaign
//                    Jeremy McMinnis, jmcminis@gmail.com, University of Illinois at Urbana-Champaign
//                    Jeongnim Kim, jeongnim.kim@gmail.com, University of Illinois at Urbana-Champaign
//                    Mark A. Berrill, berrillma@ornl.gov, Oak Ridge National Laboratory
//
// File created by: Jeongnim Kim, jeongnim.kim@gmail.com, University of Illinois at Urbana-Champaign
//////////////////////////////////////////////////////////////////////////////////////


#include "QMCDrivers/QMCOptimize.h"
#include "Particle/HDFWalkerIO.h"
#include "Particle/DistanceTable.h"
#include "OhmmsData/AttributeSet.h"
#include "Message/CommOperators.h"
#include "Optimize/CGOptimization.h"
#include "Optimize/testDerivOptimization.h"
#include "Optimize/DampedDynamics.h"
//#include "QMCDrivers/QMCCostFunctionSingle.h"
#include "QMCDrivers/VMC/VMC.h"
#include "QMCDrivers/QMCCostFunction.h"
#if defined(QMC_CUDA)
#include "QMCDrivers/VMC/VMC_CUDA.h"
#include "QMCDrivers/QMCCostFunctionCUDA.h"
#endif
#include "QMCApp/HamiltonianPool.h"

namespace qmcplusplus
{
QMCOptimize::QMCOptimize(MCWalkerConfiguration& w,
                         TrialWaveFunction& psi,
                         QMCHamiltonian& h,
                         HamiltonianPool& hpool,
                         WaveFunctionPool& ppool,
                         Communicate* comm)
    : QMCDriver(w, psi, h, ppool, comm),
      PartID(0),
      NumParts(1),
      WarmupBlocks(10),
      SkipSampleGeneration("no"),
      hamPool(hpool),
      optTarget(0),
      optSolver(0),
      vmcEngine(0),
      wfNode(NULL),
      optNode(NULL)
{
  IsQMCDriver = false;
  //set the optimization flag
  QMCDriverMode.set(QMC_OPTIMIZE, 1);
  //read to use vmc output (just in case)
  RootName = "pot";
  QMCType  = "QMCOptimize";
  //default method is cg
  optmethod = "cg";
  m_param.add(WarmupBlocks, "warmupBlocks", "int");
  m_param.add(SkipSampleGeneration, "skipVMC", "string");
}

/** Clean up the vector */
QMCOptimize::~QMCOptimize()
{
  delete vmcEngine;
  delete optSolver;
  delete optTarget;
}

/** Add configuration files for the optimization
 * @param a root of a hdf5 configuration file
 */
void QMCOptimize::addConfiguration(const std::string& a)
{
  if (a.size())
    ConfigFile.push_back(a);
}

/** Reimplement QMCDriver::run
 */
bool QMCOptimize::run()
{
  //close files automatically generated by QMCDriver
  //branchEngine->finalize();
  //generate samples
 
    /*
    m_param.add(MinMethod, "MinMethod", "string");
  
  if(MinMethod == "hybrid")
  {
      m_param.add(hybrid_descent_samples,"Hybrid_Descent_samples","int");
    optTarget->setNumSamples(hybrid_descent_samples);
  }
  */
    generateSamples();

  NumOfVMCWalkers = W.getActiveWalkers();

  //cleanup walkers
  //W.destroyWalkers(W.begin(), W.end());
  app_log() << "<opt stage=\"setup\">" << std::endl;
  app_log() << "  <log>" << std::endl;
  //reset the rootname
  optTarget->setRootName(RootName);
  optTarget->setWaveFunctionNode(wfNode);
  optTarget->setRng(vmcEngine->getRng());
  app_log() << "   Reading configurations from h5FileRoot " << std::endl;
  //get configuration from the previous run
  Timer t1;
  optTarget->getConfigurations(h5FileRoot);
  optTarget->checkConfigurations();
  app_log() << "  Execution time = " << std::setprecision(4) << t1.elapsed() << std::endl;
  app_log() << "  </log>" << std::endl;
  app_log() << "</opt>" << std::endl;
  app_log() << "<opt stage=\"main\" walkers=\"" << optTarget->getNumSamples() << "\">" << std::endl;
  app_log() << "  <log>" << std::endl;
  optTarget->setTargetEnergy(branchEngine->getEref());
  t1.restart();
  bool success = optSolver->optimize(optTarget);
  //     W.reset();
  //     branchEngine->flush(0);
  //     branchEngine->reset();
  app_log() << "  Execution time = " << std::setprecision(4) << t1.elapsed() << std::endl;
  ;
  app_log() << "  </log>" << std::endl;
  optTarget->reportParameters();

  int nw_removed = W.getActiveWalkers() - NumOfVMCWalkers;
  app_log() << "   Restore the number of walkers to " << NumOfVMCWalkers << ", removing " << nw_removed << " walkers."
            << std::endl;
  if (nw_removed > 0)
    W.destroyWalkers(nw_removed);
  else
    W.createWalkers(-nw_removed);

  app_log() << "</opt>" << std::endl;
  app_log() << "</optimization-report>" << std::endl;
  MyCounter++;
  return (optTarget->getReportCounter() > 0);
}

void QMCOptimize::generateSamples()
{
  Timer t1;
  app_log() << "<optimization-report>" << std::endl;

  vmcEngine->QMCDriverMode.set(QMC_WARMUP, 1);
  vmcEngine->QMCDriverMode.set(QMC_OPTIMIZE, 1);
  vmcEngine->QMCDriverMode.set(QMC_WARMUP, 0);

  //vmcEngine->setValue("recordWalkers",1);//set record
  vmcEngine->setValue("current", 0); //reset CurrentStep
  app_log() << "<vmc stage=\"main\" blocks=\"" << nBlocks << "\">" << std::endl;
  t1.restart();
  //     W.reset();
  branchEngine->flush(0);
  branchEngine->reset();
  vmcEngine->run();
  app_log() << "  Execution time = " << std::setprecision(4) << t1.elapsed() << std::endl;
  app_log() << "</vmc>" << std::endl;
  //write parameter history and energies to the parameter file in the trial wave function through opttarget
  EstimatorRealType e, w, var;
  vmcEngine->Estimators->getEnergyAndWeight(e, w, var);
  optTarget->recordParametersToPsi(e, var);

  h5FileRoot = RootName;
}

/** Parses the xml input file for parameter definitions for the wavefunction optimization.
 * @param q current xmlNode
 * @return true if successful
 */
bool QMCOptimize::put(xmlNodePtr q)
{
  std::string vmcMove("pbyp");
  std::string useGPU("no");
  OhmmsAttributeSet oAttrib;
  oAttrib.add(vmcMove, "move");
  oAttrib.add(useGPU, "gpu");
  oAttrib.put(q);
  xmlNodePtr qsave = q;
  xmlNodePtr cur   = qsave->children;
  int pid          = OHMMS::Controller->rank();
  while (cur != NULL)
  {
    std::string cname((const char*)(cur->name));
    if (cname == "mcwalkerset")
    {
      mcwalkerNodePtr.push_back(cur);
    }
    else if (cname.find("optimize") < cname.size())
    {
      xmlChar* att = xmlGetProp(cur, (const xmlChar*)"method");
      if (att)
      {
        optmethod = (const char*)att;
      }
      optNode = cur;
    }
    cur = cur->next;
  }
  //no walkers exist, add 10
  if (W.getActiveWalkers() == 0)
    addWalkers(omp_get_max_threads());
  NumOfVMCWalkers = W.getActiveWalkers();
  //create VMC engine
  if (vmcEngine == 0)
  {
#if defined(QMC_CUDA)
    if (useGPU == "yes")
      vmcEngine = new VMCcuda(W, Psi, H, psiPool, myComm);
    else
#endif
      vmcEngine = new VMC(W, Psi, H, psiPool, myComm);
    vmcEngine->setUpdateMode(vmcMove[0] == 'p');
  }
  vmcEngine->setStatus(RootName, h5FileRoot, AppendRun);
  vmcEngine->process(qsave);
  if (optSolver == 0)
  {
    if (optmethod == "anneal")
    {
      app_log() << " Annealing optimization using DampedDynamics" << std::endl;
      optSolver = new DampedDynamics<RealType>;
    }
    else if ((optmethod == "flexOpt") | (optmethod == "flexopt") | (optmethod == "macopt"))
    {
      app_log() << "Conjugate-gradient optimization using FlexOptimization" << std::endl;
      app_log() << " This method has been removed. " << std::endl;
      APP_ABORT("QMCOptimize::put");
    }
    else if (optmethod == "BFGS")
    {
      app_log() << " This method is not implemented correctly yet. " << std::endl;
      APP_ABORT("QMCOptimize::put");
    }
    else if (optmethod == "test")
    {
      app_log() << "Conjugate-gradient optimization using tester Optimization: " << std::endl;
      optSolver = new testDerivOptimization<RealType>;
    }
    else
    {
      app_log() << " Conjugate-gradient optimization using CGOptimization" << std::endl;
      optSolver = new CGOptimization<RealType>;
    } //set the stream
    optSolver->setOstream(&app_log());
  }
  if (optNode == NULL)
    optSolver->put(qmcNode);
  else
    optSolver->put(optNode);
  bool success = true;
  if (optTarget == 0)
  {
#if defined(QMC_CUDA)
    if (useGPU == "yes")
      optTarget = new QMCCostFunctionCUDA(W, Psi, H, myComm);
    else
#endif
      optTarget = new QMCCostFunction(W, Psi, H, myComm);
    //#if defined(ENABLE_OPENMP)
    //	if(true /*omp_get_max_threads()>1*/)
    //      {
    //        optTarget = new QMCCostFunctionOMP(W,Psi,H,hamPool);
    //      }
    //      else
    //#endif
    //        optTarget = new QMCCostFunctionSingle(W,Psi,H);
    optTarget->setStream(&app_log());
    success = optTarget->put(q);
  }
  return success;
}
} // namespace qmcplusplus
