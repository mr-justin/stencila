#include <iostream>
 
#include <boost/test/unit_test.hpp> 

#define STENCILA_CILA_PARSER_TRACE
#include <stencila/stencil-cila-parser.hpp>  

using namespace Stencila;
struct CilaParserFixture : public CilaParser {  
	// Methods added for debugging purposes
	
	void states_show(void){
		std::cout<<"-----------------States-------------------\n";
		for(auto state : states) std::cout<<state_name(state)<<"\n";
		std::cout<<"-----------------------------------------\n";
	}

	void nodes_show(void){
		std::cout<<"-----------------Nodes-------------------\n";
		for(auto node : nodes) std::cout<<node.indent.length()<<"\t"<<node.node.name()<<"\n";
		std::cout<<"-----------------------------------------\n";
	}

	void xml_show(void){
		std::cout<<"-------------------XML-------------------\n"
				<<stencil.xml()<<"\n"
				<<"-----------------------------------------\n";
	}
};

// Check macros. Macros are used so that Boost::Unit reports lines number
// of failed checks properly
#define CILA_XML(_CILA,_XML) BOOST_CHECK_EQUAL(parse(_CILA).stencil.xml(),_XML);

BOOST_FIXTURE_TEST_SUITE(cila_parser,CilaParserFixture)
 
BOOST_AUTO_TEST_CASE(elements){
	CILA_XML("div","<div />");
	CILA_XML("div\ndiv","<div /><div />");
	CILA_XML("div\na\np","<div /><a /><p />");
}

BOOST_AUTO_TEST_CASE(indentation){
	CILA_XML("div\ndiv","<div /><div />");
	CILA_XML("div\n\tp\n\t\ta\ndiv","<div><p><a /></p></div><div />");
	// Blank lines should not muck up indentation
	CILA_XML("div\n\n\tp\n\t\n  \n\n\tp\n\n \n\t\t\ta","<div><p /><p><a /></p></div>");
}

BOOST_AUTO_TEST_CASE(auto_paragraphs){
	CILA_XML("No para","No para");
	CILA_XML("\nPara","<p>Para</p>");
	CILA_XML("\n\nPara","<p>Para</p>");
	CILA_XML("div\n\n\tPara1\n\t\n\tPara2\n\t\t\tPara2cont","<div><p>Para1</p><p>Para2Para2cont</p></div>");
}

BOOST_AUTO_TEST_CASE(embedded){
	CILA_XML("div{div{div}}","<div><div><div /></div></div>");
	CILA_XML("div id=yo Some text {a href=none nowhere} after",R"(<div id="yo">Some text <a href="none">nowhere</a> after</div>)");
	CILA_XML("{ul{li apple}{li pear}}",R"(<ul><li>apple</li><li>pear</li></ul>)");
}

BOOST_AUTO_TEST_CASE(exec){
	CILA_XML("r\n\ta=1\n","<pre data-exec=\"r\">\ta=1</pre>");
}

BOOST_AUTO_TEST_CASE(sections){
	CILA_XML("> Heading",R"(<section id="heading"><h1>Heading</h1></section>)");
	CILA_XML("> Heading with spaces",R"(<section id="heading-with-spaces"><h1>Heading with spaces</h1></section>)");
}

BOOST_AUTO_TEST_CASE(ul){
	CILA_XML("- apple\n- pear",R"(<ul><li>apple</li><li>pear</li></ul>)");
	CILA_XML("-apple\n-pear",R"(<ul><li>apple</li><li>pear</li></ul>)");
	CILA_XML("{-apple}{-pear}",R"(<ul><li>apple</li><li>pear</li></ul>)");
	// List items can have normal text parsing
	CILA_XML("- Some _emphasis_",R"(<ul><li>Some <em>emphasis</em></li></ul>)");
	CILA_XML("- An interpolated ``value``",R"(<ul><li>An interpolated <span data-write="value" /></li></ul>)");
	CILA_XML("- A link to [Google](http://google.com)",R"(<ul><li>A link to <a href="http://google.com">Google</a></li></ul>)");
}

BOOST_AUTO_TEST_CASE(ol){
	CILA_XML("1. apple\n2. pear",R"(<ol><li>apple</li><li>pear</li></ol>)");
	CILA_XML("1.apple\n2.pear",R"(<ol><li>apple</li><li>pear</li></ol>)");
}

BOOST_AUTO_TEST_CASE(attributes){
	CILA_XML("div class=a",R"(<div class="a" />)");
	CILA_XML("div #an-id",R"(<div id="an-id" />)");
	CILA_XML("div .a-class",R"(<div class="a-class" />)");
	CILA_XML("a href=http://google.com #an-id .a-class",R"(<a href="http://google.com" id="an-id" class="a-class" />)");

	CILA_XML("class=a",R"(<div class="a" />)");
	CILA_XML("#an-id",R"(<div id="an-id" />)");
	CILA_XML(".a-class",R"(<div class="a-class" />)");
	CILA_XML("#an-id .a-class",R"(<div id="an-id" class="a-class" />)");
}

BOOST_AUTO_TEST_CASE(directive_no_arg){
	CILA_XML("div else",R"(<div data-else="true" />)");
	CILA_XML("else",R"(<div data-else="true" />)");
	CILA_XML("div default",R"(<div data-default="true" />)");
	CILA_XML("default",R"(<div data-default="true" />)");
}

BOOST_AUTO_TEST_CASE(directive_arg){
	CILA_XML("div write x",R"(<div data-write="x" />)");
	CILA_XML("write x",R"(<span data-write="x" />)");
	CILA_XML("div if x",R"(<div data-if="x" />)");
	CILA_XML("if x",R"(<div data-if="x" />)");
}

BOOST_AUTO_TEST_CASE(if_elif_else){
	CILA_XML("if x<0\nelif x<1\nelse",R"(<div data-if="x&lt;0" /><div data-elif="x&lt;1" /><div data-else="true" />)");
}

BOOST_AUTO_TEST_CASE(trailing_text){
	CILA_XML("div Hello",R"(<div>Hello</div>)");
	CILA_XML("a href=http://google.com Google",R"(<a href="http://google.com">Google</a>)");
	CILA_XML("div Some text with bits like #id and .class",R"(<div>Some text with bits like #id and .class</div>)");
	CILA_XML(".a-class else",R"(<div class="a-class" data-else="true" />)");
}

BOOST_AUTO_TEST_CASE(text){
	CILA_XML("","");
	CILA_XML("Hello world","Hello world");
}

BOOST_AUTO_TEST_CASE(emphasis){
	CILA_XML("_emphasised_","<em>emphasised</em>");
	CILA_XML("Some _emphasised_ text","Some <em>emphasised</em> text");
}

BOOST_AUTO_TEST_CASE(strong){
	CILA_XML("*strong*","<strong>strong</strong>");
	CILA_XML("Some *strong* text","Some <strong>strong</strong> text");
}

BOOST_AUTO_TEST_CASE(emphasis_strong){
	CILA_XML("Some _emphasised *strong* text_","Some <em>emphasised <strong>strong</strong> text</em>");
	CILA_XML("Some *strong _emphasised_ text*","Some <strong>strong <em>emphasised</em> text</strong>");
}

BOOST_AUTO_TEST_CASE(code){
	CILA_XML("`e=mc^2`","<code>e=mc^2</code>");
	CILA_XML("An escaped backtick \\` within text","An escaped backtick ` within text");
	CILA_XML("An escaped backtick within code `\\``","An escaped backtick within code <code>`</code>");
}

BOOST_AUTO_TEST_CASE(asciimath){
	CILA_XML("|e=mc^2|",R"(<span class="math"><script type="math/asciimath">e=mc^2</script></span>)");
	CILA_XML("Text before |e=mc^2|",R"(Text before <span class="math"><script type="math/asciimath">e=mc^2</script></span>)");
	CILA_XML("|e=mc^2| text after",R"(<span class="math"><script type="math/asciimath">e=mc^2</script></span> text after)");
	CILA_XML("With asterisks and underscores |a_b*c|",R"(With asterisks and underscores <span class="math"><script type="math/asciimath">a_b*c</script></span>)");
	CILA_XML("An escaped pipe within AsciiMath |a\\|b|",R"(An escaped pipe within AsciiMath <span class="math"><script type="math/asciimath">a|b</script></span>)");
}

BOOST_AUTO_TEST_CASE(tex){
	CILA_XML("\\(e=mc^2\\)",R"(<span class="math"><script type="math/tex">e=mc^2</script></span>)");
}

BOOST_AUTO_TEST_CASE(link){
	CILA_XML("[t-test](http://en.wikipedia.org/wiki/Student's_t-test)","<a href=\"http://en.wikipedia.org/wiki/Student's_t-test\">t-test</a>");
	CILA_XML("Go to [Google](http://google.com)",R"(Go to <a href="http://google.com">Google</a>)");
	CILA_XML("[Google](http://google.com) is a link",R"(<a href="http://google.com">Google</a> is a link)");
}

BOOST_AUTO_TEST_CASE(autolink){
	CILA_XML("Go to http://google.com",R"(Go to <a href="http://google.com">http://google.com</a>)");
	CILA_XML("Go to https://google.com",R"(Go to <a href="https://google.com">https://google.com</a>)");
	CILA_XML("An autolink http://google.com with text after it",R"(An autolink <a href="http://google.com">http://google.com</a> with text after it)");
}

BOOST_AUTO_TEST_CASE(interpolate){
	CILA_XML("``x``",R"(<span data-write="x" />)");
	CILA_XML("The answer is ``6*7``!",R"(The answer is <span data-write="6*7" />!)");
}

BOOST_AUTO_TEST_SUITE_END()
