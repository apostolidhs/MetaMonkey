function gen2() {
    multi = {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Identifier", name:"q"}},{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Identifier", name:"z"}}]};
    single = {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Identifier", name:"q"}}]};
    stmts = {loc:null, type:"Program", body:[{loc:null, type:"IfStatement", test:{loc:null, type:"Identifier", name:"debug"}, consequent:{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"CallExpression", callee:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"console"}, property:{loc:null, type:"Identifier", name:"log"}, computed:false}, arguments:[{loc:null, type:"Literal", value:":)"}]}}, alternate:null},{loc:null, type:"WhileStatement", test:{loc:null, type:"Literal", value:1}, body:{loc:null, type:"BreakStatement", label:null}}]};
    op = {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Identifier", name:"x"}}]};
    op2 = {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Literal", value:4}}]};
    op3 = {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"BinaryExpression", operator:"+", left:{loc:null, type:"Literal", value:1}, right:{loc:null, type:"Identifier", name:"y"}}}]};
    multiFunc = {loc:null, type:"Program", body:[{loc:null, type:"FunctionDeclaration", id:{loc:null, type:"Identifier", name:"foo8"}, params:[], defaults:[], body:{loc:null, type:"BlockStatement", body:meta_escape( true,[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Identifier", name:"x"}},{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Identifier", name:"y"}}],[{index:1,expr:multi},{index:3,expr:single}],true)}, rest:null, generator:false, expression:false},{loc:null, type:"EmptyStatement"}]};
    multiFunc2 = {loc:null, type:"Program", body:[{loc:null, type:"FunctionDeclaration", id:{loc:null, type:"Identifier", name:"foo29"}, params:[], defaults:[], body:{loc:null, type:"BlockStatement", body:meta_escape( true,[],[{index:0,expr:multi}],true)}, rest:null, generator:false, expression:false},{loc:null, type:"EmptyStatement"}]};
    singleFunc = {loc:null, type:"Program", body:[{loc:null, type:"FunctionDeclaration", id:{loc:null, type:"Identifier", name:"foo00"}, params:[], defaults:[], body:{loc:null, type:"BlockStatement", body:meta_escape( true,[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Literal", value:2}}],[{index:1,expr:single}],true)}, rest:null, generator:false, expression:false},{loc:null, type:"EmptyStatement"}]};
    singleExpr = {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"CallExpression", callee:{loc:null, type:"Identifier", name:"print"}, arguments:meta_escape( true,[{loc:null, type:"Literal", value:3}],[{index:1,expr:single}],false)}}]};
    multiExpr = {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"CallExpression", callee:{loc:null, type:"Identifier", name:"print"}, arguments:meta_escape( true,[{loc:null, type:"Literal", value:3}],[{index:0,expr:multi}],false)}}]};
    return [{loc:null, type:"Program", body:meta_escape( true,[{loc:null, type:"VariableDeclaration", kind:"var", declarations:[{loc:null, type:"VariableDeclarator", id:{loc:null, type:"Identifier", name:"tt"}, init:{loc:null, type:"BinaryExpression", operator:"+", left:meta_escape( false,op,false), right:meta_escape( false,op3,false)}}]}],[{index:1,expr:stmts},{index:2,expr:multiFunc}],true)}];
}

(var tt = x + (1 + y);
if (debug)
    console.log(":)");
while (1)
    break;
function foo8() {
    x;
    q;
    z;
    y;
    q;
}

;
);
