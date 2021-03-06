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



show("state-field").when("country").is("United States");