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

  static ExprSet unionExprSet (const ExprSet& e1, const ExprSet& e2) {
    ExprSet result = e1;
    for (auto i2 : e2) {
      result.insert(i2);
    }
    return result;
  }

  static ExprSet subtractExprSet (const ExprSet& e1, const ExprSet& e2) {
    ExprSet result = e1;
    for (auto i2 : e2) {
      result.erase(i2);
    }
    return result;
  }

  static void printExprSet (ExprSet es) {
    std::cout << std::endl;
    for (auto e : es) {
      std::cout << "      " << e << std::endl;
    }
  }

  static void printTable (Table *table) {
    for (auto k : table->keys)
      std::cout << "    Key: " << k << std::endl;
    for (auto a : table->actions)
      std::cout << "    Action: " << a.first << std::endl;
  }


  Action::Action() : action(nullptr), def({}), use({}) {}

  TableAnalyzer::TableAnalyzer(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
    : refMap(refMap), typeMap(typeMap), curAction(new Action()), 
      curActionMap(new ActionMap()), curTable(new Table()){}

  void TableAnalyzer::setCurrentAction(const IR::P4Action *action) {
    curAction->action = action;
    curAction->def = {};
    curAction->use = {};
  }

  void TableAnalyzer::saveCurrentAction() {
    (*curActionMap)[curAction->action->toString()] = curAction;
    curAction = new Action();
  }

  void TableAnalyzer::clearCurrentActionMap() {
    delete(curActionMap);
    curActionMap = new ActionMap();
  }

  ExprSet TableAnalyzer::findId(const IR::Expression *expr) {
    if (expr->is<IR::ListExpression>()) {
      auto exprList = expr->to<IR::ListExpression>()->components;
      if (exprList.empty()) {
        return {};
      } else {
        auto lastExpr = exprList.back();
        exprList.pop_back();
        const IR::Expression *rest = new IR::ListExpression(exprList);
        return unionExprSet(findId(lastExpr), findId(rest));
      }
    }
    if (expr->is<IR::Operation_Binary>()) {
      const IR::Operation_Binary *bexpr = expr->to<IR::Operation_Binary>();
      ExprSet lresult = findId(bexpr->left);
      ExprSet rresult = findId(bexpr->right);
      return unionExprSet(lresult, rresult);
    }
    if (expr->is<IR::Operation_Ternary>()) {
      const IR::Operation_Ternary *texpr = expr->to<IR::Operation_Ternary>();
      ExprSet e0r = findId(texpr->e0);
      ExprSet e1r = findId(texpr->e1);
      ExprSet e2r = findId(texpr->e2);
      return unionExprSet(unionExprSet(e0r, e1r), e2r);
    }
    if (expr->is<IR::Operation_Unary>()) {
      if (expr->is<IR::Member>()) {
        auto m = expr->to<IR::Member>();
        if (m->expr->is<IR::TypeNameExpression>()) {
          return {};
        } else {
          ExprSet result = {expr->toString()};
          return result;
        }
      } else {
      return findId(expr->to<IR::Operation_Unary>()->expr);
      }
    }
    if (expr->is<IR::AttribLocal>()) {
      ExprSet result = {expr->toString()};
      return result;
    }
    ExprSet v = {};
    return v;
  }

  bool TableAnalyzer::preorder(const IR::PackageBlock *block) {
    for (auto it : block->constantValue) {
      if(it.second->is<IR::ControlBlock>()) {
        auto name = it.second->to<IR::ControlBlock>()->container->name;
        std::cout << "\nAnalyzing top-level control " << name << std::endl;
        visit(it.second->getNode());
        clearCurrentActionMap();
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

  void TableAnalyzer::visitExterns(const P4::MethodInstance *instance) {
    auto args = instance->expr->arguments;
    auto params = instance->getActualParameters();
    ExprSet inExprs = {};
    ExprSet outExprs = {};
    
    //std::cout << "    Extern: " << instance->expr->method << std::endl;
    if (args->size() != params->size()) {
      std::cout << "ERROR: the number of args / params does not match" << std::endl;
      return;
    }

    for (unsigned i = 0; i < args->size(); i++) {
      auto a = (*args)[i];
      auto p = params->getParameter(i);
      if (p->hasOut()){
        //std::cout << "      OUT: " << a << std::endl;
        outExprs = unionExprSet(findId(a), outExprs);
      } else {
        //std::cout << "      IN:  " << a << std::endl;
        inExprs = unionExprSet(findId(a), inExprs);
      }
    }

    if (curAction->action != nullptr) {
      curAction->def = unionExprSet(curAction->def, outExprs);
      curAction->use = unionExprSet(curAction->use,
          subtractExprSet(inExprs, curAction->def));
    }
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
    else if (curAction->action != nullptr && instance->is<P4::ExternFunction>()) {
      visitExterns(instance);
    }
    else if (curAction->action != nullptr && instance->is<P4::ExternMethod>()) {
      visitExterns(instance);
    }
    return false;
  }

  bool TableAnalyzer::preorder(const IR::AssignmentStatement *statement) {
    if (curAction->action != nullptr) {
      if (statement->left->is<IR::Member>()) {
        curAction->def = unionExprSet(curAction->def, {statement->left->toString()});
      } else if (statement->left->is<IR::AttribLocal>()) {
        curAction->def = unionExprSet(curAction->def, {statement->left->toString()});
      }
      curAction->use = unionExprSet(curAction->use, 
          subtractExprSet(findId(statement->right), curAction->def));
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
    std::cout << "  P4Table: " << table->controlPlaneName() << std::endl;

    //Key
    const auto keys = table->getKey();
    if (keys != nullptr) {
      if (keys->keyElements.empty() == false) {
        //std::cout << "Keys:" << std::endl;
        for (const auto key : keys->keyElements)
          visit(key);
      }
    }

    //Action
    const auto actions = table->getActionList();
    if (actions != nullptr) {
      if (actions->actionList.empty() == false) {
        //std::cout << "Actions:" << std::endl;
        for (const auto action : actions->actionList)
          visit(action);
      }
    }

    printTable(curTable);
    curTable = new Table();
    return false;
  }

  bool TableAnalyzer::preorder(const IR::ActionListElement *action) {
    auto a = refMap->getDeclaration(action->getPath(), true)->to<IR::P4Action>();
    //std::cout << "  ActionListElement: " << a->toString() << std::endl;
    
    if ((*curActionMap)[a->toString()] != nullptr) {
      curTable->actions[a->toString()] = (*curActionMap)[a->toString()];
    }
    return false;
  }

  bool TableAnalyzer::preorder(const IR::P4Action *action) {
    std::cout << "  P4Action: " << action->toString() << std::endl;
    setCurrentAction(action);
    visit(action->body);
    std::cout << "    Def: ";
    printExprSet(curAction->def);
    std::cout << "    Use: ";
    printExprSet(curAction->use);
    saveCurrentAction();
    return false;
  }

  bool TableAnalyzer::preorder(const IR::KeyElement *key) {
    if (key->expression != nullptr) {
      //std::cout << "  Key: " << key->expression->toString() << std::endl;
      curTable->keys.insert(key->expression->toString());
    }
    return false;
  }

} //namsepace multip4
