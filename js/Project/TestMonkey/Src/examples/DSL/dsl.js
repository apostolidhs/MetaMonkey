// show usstate field when country select box is "united states"

// show("usstate").when("country").is("united states");



function DependencyTrigger(elementId, dependency){
	this.element = $(elementId);
	this.dependency = dependency;

	this.is = function(values){
		this.values = values;
		this.addHandler();
		console.log( 
				[ "showing", this.dependency.element.id, "when", this.element.id, "is", this.values  ].join(' ')
			);
	};

	this.addHandler = function(){
		Event.observe( this.element, "change", this.checkDependency.bind(this) );
	};

	this.checkDependency = function(){
		console.log( 
				[ "checking", this.element.id, "for", this.values ].join(' ')
			);

		if(this.values.split(",").indexOf($F(this.element)) > -1){
			this.dependency.element.show();
		} else {
			this.dependency.element.hide();
		}
	};
}

function Dependency(elementId){
	this.element = $(elementId);

	this.when = function(elementId){
		return new DependencyTrigger(elementId, this);
	};
}

function show(elementId){
	return new Dependency(elementId);
}


function jsonToAst( jsonFilename ){

	function Ast_esc(ast, isStmt){
		return isStmt? ast.body[0] : ast.body[0].expression;
	}

	function Ast_GetBody(root){
		return root.body[0].expression.callee.body.body;
	}

	var dslParser = {
		parseItems:{
			show: function(obj){
				var value = obj.value;
				var whenObj = obj.when;

				var retVal = .<
							(function(){
								var dependency = new Dependency("");
								.~this.when(whenObj, .< dependency; >.);
								return dependency;
							})();
						>.;	
				var retValBody = Ast_GetBody(retVal);
				retValBody[0].declarations[0].init.arguments[0].value = value;

				return retVal;
			},

			when: function(obj, parent){
				var value = obj.value;
				var isObj = obj.is;

				var retVal = .<
							(function(parent){
								var dependencyTrigger = new DependencyTrigger("", parent);
								.~this.is(isObj, .< dependencyTrigger; >.);
							})(.~parent);
						>.;	
				var retValBody = Ast_GetBody(retVal);
				retValBody[0].declarations[0].init.arguments[0].value = value;
				
				return retVal;
			},

			is: function(obj, parent){

				var value = obj.value;
				var retVal = .<
							(function(parent){
								parent.values = "";
								parent.addHandler();
								console.log( 
										[ "showing", parent.dependency.element.id, "when", parent.element.id, "is", parent.values  ].join(' ')
									);
							})(.~parent);
						>.;	
				var retValBody = Ast_GetBody(retVal);
				retValBody[0].expression.right.value = value;

				return retVal;
			}//,

			// call: function(dslKey){
			// 	var handler = this[dslKey];
			// 	assert(handler);
			// 	return this[queryKey]()
			// }
		},
		startParser: function(obj){
			var showObj = obj.show;
			return this.parseItems.show(showObj);
		}
	};

	var jsonDsl = read(jsonFilename);
	var dslObj = JSON.parse(jsonDsl);
	return dslParser.startParser(dslObj);
}


.!jsonToAst(
		"Src\\examples\\DSL\\dsl.json"
	);