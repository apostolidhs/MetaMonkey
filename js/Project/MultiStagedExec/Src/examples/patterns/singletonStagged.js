function genSingleton(code) {
    return {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"CallExpression", callee:{loc:null, type:"FunctionExpression", id:null, params:[], defaults:[], body:{loc:null, type:"BlockStatement", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Literal", value:"use strict"}},{loc:null, type:"VariableDeclaration", kind:"var", declarations:[{loc:null, type:"VariableDeclarator", id:{loc:null, type:"Identifier", name:"instance"}, init:null}]},{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"AssignmentExpression", operator:"=", left:{loc:null, type:"Identifier", name:"myclass"}, right:{loc:null, type:"FunctionExpression", id:null, params:[{loc:null, type:"Identifier", name:"args"}], defaults:[], body:{loc:null, type:"BlockStatement", body:meta_escape( true,[{loc:null, type:"IfStatement", test:{loc:null, type:"Identifier", name:"instance"}, consequent:{loc:null, type:"BlockStatement", body:[{loc:null, type:"ReturnStatement", argument:{loc:null, type:"Identifier", name:"instance"}}]}, alternate:null},{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"AssignmentExpression", operator:"=", left:{loc:null, type:"Identifier", name:"instance"}, right:{loc:null, type:"ThisExpression"}}}],[{index:2,expr:code}],true)}, rest:null, generator:false, expression:false}}},{loc:null, type:"ReturnStatement", argument:{loc:null, type:"Identifier", name:"myclass"}}]}, rest:null, generator:false, expression:false}, arguments:[]}}]};
}

var myclass = (function () {
    "use strict";
    var instance;
    myclass = function (args) {
    if (instance) {
    return instance;
    }
    instance = this;
    console.log("hello world");
    }
;
    return myclass;
}
());
;
var class1 = new myclass;
var class2 = new myclass;
console.log(class1 === class2);
