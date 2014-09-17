function DependencyTrigger(elementId, dependency) {
    this.element = $(elementId);
    this.dependency = dependency;
    this.is = function (values) {
    this.values = values;
    this.addHandler();
    console.log(["showing", this.dependency.element.id, "when", this.element.id, "is", this.values].join(" "));
    }
;
    this.addHandler = function () {
    Event.observe(this.element, "change", this.checkDependency.bind(this));
    }
;
    this.checkDependency = function () {
    console.log(["checking", this.element.id, "for", this.values].join(" "));
    if (this.values.split(",").indexOf($F(this.element)) > -1) {
    this.dependency.element.show();
    } else {
    this.dependency.element.hide();
    }
    }
;
}

function Dependency(elementId) {
    this.element = $(elementId);
    this.when = function (elementId) {
    return (new DependencyTrigger(elementId, this));
    }
;
}

function show(elementId) {
    return (new Dependency(elementId));
}

function jsonToAst(jsonFilename) {
    function Ast_esc(ast, isStmt) {
    return (isStmt?ast.body[0]:ast.body[0].expression);
    }

    function Ast_GetBody(root) {
    return root.body[0].expression.callee.body.body;
    }

    var dslParser = {parseItems: {show: function (obj) {
    var value = obj.value;
    var whenObj = obj.when;
    return {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"CallExpression", callee:{loc:null, type:"FunctionExpression", id:null, params:[], defaults:[], body:{loc:null, type:"BlockStatement", body:meta_escape( true,[{loc:null, type:"VariableDeclaration", kind:"var", declarations:[{loc:null, type:"VariableDeclarator", id:{loc:null, type:"Identifier", name:"dependency"}, init:{loc:null, type:"NewExpression", callee:{loc:null, type:"Identifier", name:"Dependency"}, arguments:[meta_escapejsvalue( value)]}}]},{loc:null, type:"ReturnStatement", argument:{loc:null, type:"Identifier", name:"dependency"}}],[{index:1,expr:this.when(whenObj, {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Identifier", name:"dependency"}}]})}],true)}, rest:null, generator:false, expression:false}, arguments:[]}}]};
}
,when: function (obj, parent) {
    var value = obj.value;
    var isObj = obj.is;
    return {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"CallExpression", callee:{loc:null, type:"FunctionExpression", id:null, params:[{loc:null, type:"Identifier", name:"parent"}], defaults:[], body:{loc:null, type:"BlockStatement", body:meta_escape( true,[{loc:null, type:"VariableDeclaration", kind:"var", declarations:[{loc:null, type:"VariableDeclarator", id:{loc:null, type:"Identifier", name:"dependencyTrigger"}, init:{loc:null, type:"NewExpression", callee:{loc:null, type:"Identifier", name:"DependencyTrigger"}, arguments:[meta_escapejsvalue( value),{loc:null, type:"Identifier", name:"parent"}]}}]}],[{index:1,expr:this.is(isObj, {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Identifier", name:"dependencyTrigger"}}]})}],true)}, rest:null, generator:false, expression:false}, arguments:meta_escape( true,[],[{index:0,expr:parent}],false)}}]};
}
,is: function (obj, parent) {
    var value = obj.value;
    return {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"CallExpression", callee:{loc:null, type:"FunctionExpression", id:null, params:[{loc:null, type:"Identifier", name:"parent"}], defaults:[], body:{loc:null, type:"BlockStatement", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"AssignmentExpression", operator:"=", left:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"parent"}, property:{loc:null, type:"Identifier", name:"values"}, computed:false}, right:meta_escapejsvalue( value)}},{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"CallExpression", callee:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"parent"}, property:{loc:null, type:"Identifier", name:"addHandler"}, computed:false}, arguments:[]}},{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"CallExpression", callee:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"console"}, property:{loc:null, type:"Identifier", name:"log"}, computed:false}, arguments:[{loc:null, type:"CallExpression", callee:{loc:null, type:"MemberExpression", object:{loc:null, type:"ArrayExpression", elements:[{loc:null, type:"Literal", value:"showing"},{loc:null, type:"MemberExpression", object:{loc:null, type:"MemberExpression", object:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"parent"}, property:{loc:null, type:"Identifier", name:"dependency"}, computed:false}, property:{loc:null, type:"Identifier", name:"element"}, computed:false}, property:{loc:null, type:"Identifier", name:"id"}, computed:false},{loc:null, type:"Literal", value:"when"},{loc:null, type:"MemberExpression", object:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"parent"}, property:{loc:null, type:"Identifier", name:"element"}, computed:false}, property:{loc:null, type:"Identifier", name:"id"}, computed:false},{loc:null, type:"Literal", value:"is"},{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"parent"}, property:{loc:null, type:"Identifier", name:"values"}, computed:false}]}, property:{loc:null, type:"Identifier", name:"join"}, computed:false}, arguments:[{loc:null, type:"Literal", value:" "}]}]}}]}, rest:null, generator:false, expression:false}, arguments:meta_escape( true,[],[{index:0,expr:parent}],false)}}]};
}
},startParser: function (obj) {
    var showObj = obj.show;
    return this.parseItems.show(showObj);
}
};
    var jsonDsl = read(jsonFilename);
    var dslObj = JSON.parse(jsonDsl);
    return dslParser.startParser(dslObj);
}

(function () {
    var dependency = new Dependency("state-field");
    (function (parent) {
    var dependencyTrigger = new DependencyTrigger("country", parent);
    (function (parent) {
    parent.values = "United States";
    parent.addHandler();
    console.log(["showing", parent.dependency.element.id, "when", parent.element.id, "is", parent.values].join(" "));
    }
(dependencyTrigger));
    }
(dependency));
    return dependency;
}
());
;
