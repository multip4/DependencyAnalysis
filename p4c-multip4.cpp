/*
Written by Seungbin Song
*/

#include "ir/ir.h"
#include "lib/log.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/crash.h"
#include "lib/nullstream.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/frontend.h"

#include "tableAnalyzer.h"

namespace multip4 {
  
  class Options : public CompilerOptions {};

  using Multip4Context = P4CContextWithOptions<Options>;

  class MidEnd : public PassManager {
    public:
      P4::ReferenceMap    refMap;
      P4::TypeMap         typeMap;
      IR::ToplevelBlock   *toplevel = nullptr;

      explicit MidEnd(CompilerOptions& options);
      IR::ToplevelBlock* process(const IR::P4Program *&program) {
          program = program->apply(*this);
          return toplevel;
      }
  };

  MidEnd::MidEnd(CompilerOptions& options) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    refMap.setIsV1(isv1);
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    setName("MidEnd");

    addPasses({
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); }),
    });
  } 

} //namespace multip4

int main(int argc, char *const argv[]) {
	setup_gc_logging();
  setup_signals();

  AutoCompileContext autoMultip4Context(new ::multip4::Multip4Context);
  auto& options = ::multip4::Multip4Context::get().options();
  options.langVersion = CompilerOptions::FrontendVersion::P4_16;
  options.compilerVersion = "0.0.1";

  if (options.process(argc, argv) != nullptr)
    options.setInputFile();
  if(::errorCount() > 0)
    return 1;

  auto hook = options.getDebugHook();

  auto program = P4::parseP4File(options);
  if (program == nullptr || ::errorCount() > 0)
    return 1;

  try {
    P4::P4COptionPragmaParser optionsPragmaParser;
    program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

    P4::FrontEnd fe;
    fe.addDebugHook(hook);
    program = fe.run(options, program);
  } catch (const Util::P4CExceptionBase &bug) {
    std::cerr << bug.what() << std::endl;
    return 1;
  }
  if (program == nullptr || ::errorCount() > 0)
    return 1;

  multip4::MidEnd midEnd(options);
  midEnd.addDebugHook(hook);
  const IR::ToplevelBlock *top = nullptr;
  try {
    top = midEnd.process(program);
    if (options.dumpJsonFile)
        JSONGenerator(*openFile(options.dumpJsonFile, true)) << program << std::endl;
  } catch (const Util::P4CExceptionBase &bug) {
    std::cerr << bug.what() << std::endl;
    return 1;
  }
  if (::errorCount() > 0)
    return 1;

  std::cout << "Generating match-action dependency graphs" << std::endl;
  multip4::TableAnalyzer ta(&midEnd.refMap, &midEnd.typeMap, options.file);
  top->getMain()->apply(ta);

  return ::errorCount() > 0;

}
