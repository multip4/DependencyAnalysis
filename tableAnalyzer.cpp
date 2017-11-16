/* 
Written by Seungbin Song 
*/


#include "tableAnalyzer.h"

#include "frontends/p4/methodInstance.h"
#include "frontends/p4/tableApply.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "lib/path.h"

namespace multip4 {

  static Expressions unionExpressions (const Expressions& e1, const Expressions& e2) {
    Expressions result = e1;
    result.insert(e2.cbegin(), e2.cend());
    return result;
  }

  static Expressions subtractExpressions (const Expressions& e1, const Expressions& e2) {
    Expressions result = e1;
    for (auto i : e2)
      result.erase(i);
    return result;
  }

  static void printExpressions (Expressions es) {
    for (auto e : es) {
      e->dbprint(std::cout);
      std::cout << " | ";
    }
    std::cout << std::endl;
  }

  ActionSet::ActionSet() : action(nullptr), def({}), use({}) {}

  TableAnalyzer::TableAnalyzer(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
    : refMap(refMap), typeMap(typeMap), curAction(new ActionSet()) {}

  void TableAnalyzer::setCurrentAction(const IR::P4Action *action) {
    curAction->action = action;
    curAction->def = {};
    curAction->use = {};
  }

  void TableAnalyzer::clearCurrentAction() {
    curAction->action = nullptr;
    curAction->def = {};
    curAction->use = {};
  }

  Expressions TableAnalyzer::findId(const IR::Expression *expr) {
    if (expr->is<IR::Operation_Binary>()) {
      const IR::Operation_Binary *bexpr = expr->to<IR::Operation_Binary>();
      Expressions lresult = findId(bexpr->left);
      Expressions rresult = findId(bexpr->right);
      return unionExpressions(lresult, rresult);
    }
    if (expr->is<IR::Operation_Ternary>()) {
      const IR::Operation_Ternary *texpr = expr->to<IR::Operation_Ternary>();
      Expressions e0r = findId(texpr->e0);
      Expressions e1r = findId(texpr->e1);
      Expressions e2r = findId(texpr->e2);
      return unionExpressions(unionExpressions(e0r, e1r), e2r);
    }
    if (expr->is<IR::Member>()) {
      Expressions result = {expr};
      return result;
    }
    if (expr->is<IR::AttribLocal>()) {
      Expressions result = {expr};
      return result;
    }
    Expressions v = {};
    return v;
  }

  bool TableAnalyzer::preorder(const IR::PackageBlock *block) {
    for (auto it : block->constantValue) {
      if(it.second->is<IR::ControlBlock>()) {
        auto name = it.second->to<IR::ControlBlock>()->container->name;
        std::cout << "Analyzing top-level control " << name << std::endl;
        visit(it.second->getNode());
      }
    }
    return false;
  }


  bool TableAnalyzer::preorder(const IR::ControlBlock *block) {
    visit(block->container);
    return false;
  }

  bool TableAnalyzer::preorder(const IR::P4Control *cont) {
    auto decls = cont->getDeclarations();
    for (auto i = decls->begin(); i != decls->end(); ++i) {
      auto p4action = (*i)->to<IR::P4Action>();
      visit(p4action);
    }
    visit(cont->body);
    return false;
  }

  bool TableAnalyzer::preorder(const IR::BlockStatement *statement) {
    for (const auto component : statement->components) {
      visit(component);
    }
    return false;
  }

  bool TableAnalyzer::preorder(const IR::IfStatement *statement) {
    visit(statement->ifTrue);
    if(statement->ifFalse != nullptr) {
      visit(statement->ifFalse);
    }
    return false;
  }

  bool TableAnalyzer::preorder(const IR::SwitchStatement *statement) {
    auto tbl = P4::TableApplySolver::isActionRun(statement->expression, refMap, typeMap);
    if (tbl != nullptr) {
      visit(tbl);
    }

    for (auto scase : statement->cases) {
      if(scase->statement != nullptr) {
        visit(scase->statement);
      }
      if(scase->label->is<IR::DefaultExpression>()) {
        break;
      }
    }

    return false;
  }

  bool TableAnalyzer::preorder(const IR::MethodCallStatement *statement) {
    auto instance = P4::MethodInstance::resolve(statement->methodCall, refMap, typeMap);
    if(instance->is<P4::ApplyMethod>()) {
      auto am = instance->to<P4::ApplyMethod>();
      if (am->object->is<IR::P4Table>()) {
        visit(am->object->to<IR::P4Table>());
      } else if (am->applyObject->is<IR::Type_Control>()) {
        if (am->object->is<IR::Parameter>()) {
          ::error("%1%: control parameters ar not supported by this target", am->object);
          return false;
        } 
        BUG_CHECK(am->object->is<IR::Declaration_Instance>(),
            "Unsupported control invocation: %1%", am->object);
        auto instantiation = am->object->to<IR::Declaration_Instance>();
        auto type = instantiation->type;
        if (type->is<IR::Type_Name>()) {
          auto tn = type->to<IR::Type_Name>();
          auto decl = refMap->getDeclaration(tn->path, true);
          visit(decl->to<IR::P4Control>());
        }   
      } else {
          BUG("Unsupported apply method: %1%", instance);
      }
    }
    return false;
  }

  bool TableAnalyzer::preorder(const IR::AssignmentStatement *statement) {
    //std::cout << "    Assignment: ";
    //statement->dbprint(std::cout);
    //std::cout << std::endl;

    if (curAction == nullptr)
      std::cout << "current action is nullptr." << std::endl;
    else {
      //std::cout << "    Left expression: ";
      //statement->left->dbprint(std::cout);
      //std::cout << std::endl;
      if (statement->left->is<IR::Member>()) {
        curAction->def = unionExpressions(curAction->def, {statement->left});
      } else if (statement->left->is<IR::AttribLocal>()) {
        curAction->def = unionExpressions(curAction->def, {statement->left});
      }

      //std::cout << "    Right expressions:" << std::endl;
      //std::cout << "      ";
      //printExpressions(findId(statement->right));
      curAction->use = unionExpressions(curAction->use, 
          subtractExpressions(findId(statement->right), curAction->def));
    }
    return false;
  }

  bool TableAnalyzer::preorder(const IR::ReturnStatement *) {
    return false;
  }

  bool TableAnalyzer::preorder(const IR::ExitStatement *) {
    return false;
  }

  bool TableAnalyzer::preorder(const IR::P4Table *table) {
    std::cout << "Table: " << table->controlPlaneName() << std::endl;

    //Key
    const auto keys = table->getKey();
    if (keys != nullptr) {
      if (keys->keyElements.empty() == false) {
        std::cout << "Keys:" << std::endl;
        for (const auto key : keys->keyElements)
          visit(key);
      }
    }

    //Action
    const auto actions = table->getActionList();
    if (actions != nullptr) {
      if (actions->actionList.empty() == false) {
        std::cout << "Actions:" << std::endl;
        for (const auto action : actions->actionList)
          visit(action);
      }
    }


    return false;
  }

  bool TableAnalyzer::preorder(const IR::ActionListElement *action) {
    std::cout << "  Action: " << action->toString() << std::endl;
    return false;
  }

  bool TableAnalyzer::preorder(const IR::P4Action *action) {
    std::cout << "  P4Action: " << action->toString() << std::endl;
    setCurrentAction(action);
    visit(action->body);
    std::cout << "  Def: ";
    printExpressions(curAction->def);
    std::cout << "  Use: ";
    printExpressions(curAction->use);
    clearCurrentAction();
    return false;
  }

  bool TableAnalyzer::preorder(const IR::KeyElement *key) {
    if (key->expression != nullptr)
      std::cout << "  Key: " << key->expression->toString() << std::endl;

    return false;
  }

} //namsepace multip4
