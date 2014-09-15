function memoized(funDef) {
    return {loc:null, type:"Program", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"CallExpression", callee:{loc:null, type:"FunctionExpression", id:null, params:[{loc:null, type:"Identifier", name:"funDef"}], defaults:[], body:{loc:null, type:"BlockStatement", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"Literal", value:"use strict"}},{loc:null, type:"VariableDeclaration", kind:"var", declarations:[{loc:null, type:"VariableDeclarator", id:{loc:null, type:"Identifier", name:"funcMemoized"}, init:{loc:null, type:"FunctionExpression", id:null, params:[], defaults:[], body:{loc:null, type:"BlockStatement", body:[{loc:null, type:"VariableDeclaration", kind:"var", declarations:[{loc:null, type:"VariableDeclarator", id:{loc:null, type:"Identifier", name:"cacheKey"}, init:{loc:null, type:"CallExpression", callee:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"JSON"}, property:{loc:null, type:"Identifier", name:"stringify"}, computed:false}, arguments:[{loc:null, type:"CallExpression", callee:{loc:null, type:"MemberExpression", object:{loc:null, type:"MemberExpression", object:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"Array"}, property:{loc:null, type:"Identifier", name:"prototype"}, computed:false}, property:{loc:null, type:"Identifier", name:"slice"}, computed:false}, property:{loc:null, type:"Identifier", name:"call"}, computed:false}, arguments:[{loc:null, type:"Identifier", name:"arguments"}]}]}}]},{loc:null, type:"VariableDeclaration", kind:"var", declarations:[{loc:null, type:"VariableDeclarator", id:{loc:null, type:"Identifier", name:"result"}, init:null}]},{loc:null, type:"IfStatement", test:{loc:null, type:"UnaryExpression", operator:"!", argument:{loc:null, type:"MemberExpression", object:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"funcMemoized"}, property:{loc:null, type:"Identifier", name:"cache"}, computed:false}, property:{loc:null, type:"Identifier", name:"cacheKey"}, computed:true}, prefix:true}, consequent:{loc:null, type:"BlockStatement", body:[{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"AssignmentExpression", operator:"=", left:{loc:null, type:"Identifier", name:"result"}, right:{loc:null, type:"CallExpression", callee:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"funDef"}, property:{loc:null, type:"Identifier", name:"apply"}, computed:false}, arguments:[{loc:null, type:"Literal", value:null},{loc:null, type:"Identifier", name:"arguments"}]}}},{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"AssignmentExpression", operator:"=", left:{loc:null, type:"MemberExpression", object:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"funcMemoized"}, property:{loc:null, type:"Identifier", name:"cache"}, computed:false}, property:{loc:null, type:"Identifier", name:"cacheKey"}, computed:true}, right:{loc:null, type:"Identifier", name:"result"}}}]}, alternate:null},{loc:null, type:"ReturnStatement", argument:{loc:null, type:"MemberExpression", object:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"funcMemoized"}, property:{loc:null, type:"Identifier", name:"cache"}, computed:false}, property:{loc:null, type:"Identifier", name:"cacheKey"}, computed:true}}]}, rest:null, generator:false, expression:false}}]},{loc:null, type:"ExpressionStatement", expression:{loc:null, type:"AssignmentExpression", operator:"=", left:{loc:null, type:"MemberExpression", object:{loc:null, type:"Identifier", name:"funcMemoized"}, property:{loc:null, type:"Identifier", name:"cache"}, computed:false}, right:{loc:null, type:"ObjectExpression", properties:[]}}},{loc:null, type:"ReturnStatement", argument:{loc:null, type:"Identifier", name:"funcMemoized"}}]}, rest:null, generator:false, expression:false}, arguments:meta_escape( true,[],[{index:0,expr:funDef}],false)}}]};
}

function cos(x) {
    return Math.cos(x);
}

cos = (function (funDef) {
    "use strict";
    var funcMemoized = function () {
    var cacheKey = JSON.stringify(Array.prototype.slice.call(arguments));
    var result;
    if (!funcMemoized.cache[cacheKey]) {
    result = funDef.apply(null, arguments);
    funcMemoized.cache[cacheKey] = result;
    }
    return funcMemoized.cache[cacheKey];
}
;
    funcMemoized.cache = {};
    return funcMemoized;
}
(cos));
;
