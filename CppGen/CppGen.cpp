#include "../glslang/Public/ShaderLang.h"
#include "../glslang/Include/Types.h"
#include "../StandAlone/ResourceLimits.h"

#include <optional>
#include <memory>
#include <cassert>

static void processArguments(int argc, char** argv);

struct InputShaderFile
{
	std::string filePath;
	EShLanguage stage;
	std::string contents;

	InputShaderFile(const std::string& filePath, EShLanguage stage);

	std::shared_ptr<glslang::TShader> tshader;
};

InputShaderFile::InputShaderFile(const std::string& filePath, EShLanguage stage)
	: filePath(filePath), stage(stage)
{ }

static std::vector<InputShaderFile> sShaderFiles;

static std::optional<std::string> readFile(const std::string& fileName)
{
	std::FILE* file = std::fopen (fileName.c_str (), "r");

	if (!file)
		return { };

	std::fseek(file, 0, SEEK_END);
	const long fileSize = std::ftell(file);
	std::fseek(file, 0, SEEK_SET);

	std::string text;
	text.resize(fileSize);

	std::fread(text.data(), sizeof(char), fileSize, file);
	std::fclose(file);

	return text;
}

static TBuiltInResource sResources = glslang::DefaultTBuiltInResource;

static void CompileFile(InputShaderFile& shader)
{
	char* text = shader.contents.data();

	shader.tshader = std::make_shared<glslang::TShader>(shader.stage);

	glslang::TShader* tshader = shader.tshader.get();

	tshader->setStrings(&text, 1);
	const bool ok = shader.tshader->parse(&sResources, 110, false, EShMessages::EShMsgDefault);

	if (ok)
		std::printf("Successfully compiled shader '%s'\n", shader.filePath.c_str());
	else {
		std::string err = tshader->getInfoLog();

		std::fprintf(stderr, "%s", err.c_str());
		std::fprintf(stderr, "Errors occurred whilst compiling shader '%s'\n", shader.filePath.c_str());
	}
}

int main(int argc, char** argv) {
	processArguments (argc, argv);

	ShInitialize();

	for (InputShaderFile& s : sShaderFiles) {
		const auto file = readFile(s.filePath);

		if (file)
			s.contents = file.value();
		else {
			std::fprintf(stderr, "Cannot open shader file '%s'\n", s.filePath.c_str());
			std::exit(1);
		}
	}

	for (InputShaderFile& s : sShaderFiles)
		CompileFile(s);

	using namespace glslang;

	TProgram pgrm;
	for (InputShaderFile& s : sShaderFiles) {
		TShader* tshader = s.tshader.get();
		pgrm.addShader(tshader);
	}

	const bool okLink = pgrm.link(EShMsgDefault);
		const std::string err = pgrm.getInfoLog();
		std::fprintf(stderr, "%s", err.c_str());
	if (!okLink) {
		std::fprintf(stderr, "Errors occurred whilst linking shaders...\n");
		std::exit(1);
	}
	else {
		std::printf("Successfully linked shaders!\n");
	}

	const bool okReflectBuild = pgrm.buildReflection();
	assert (okReflectBuild);

	for (int i = 0; i < pgrm.getNumUniformVariables(); ++i) {
		TObjectReflection obj = pgrm.getUniform(i);
		const TType* t = obj.getType();
		std::printf("Identified uniform: %s (%s)\n", obj.name.c_str(), "");
	}

	ShFinalize();
	return 0;
}

static std::string getArgSafe(int argc, char** argv, int i)
{
	if (i >= argc || i < 0)
		return std::string ();

	return argv[i];
}

static void badArgExit(const char* arg)
{
	std::fprintf (stderr, "Unknown argument '%s'\n", arg);
	std::exit (1);
}

static void processArguments(int argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		auto colonIndex = arg.find ('@');
	
		if (colonIndex == std::string::npos) {
			std::fprintf (stderr, "Bad input file syntax - expected <filepath>:<stage>\n");
			std::exit (1);
		}

		const std::string filepath = arg.substr (0, colonIndex);
		const std::string stageStr = arg.substr (colonIndex+1);

		if (filepath.empty () || stageStr.empty ()) {
			std::fprintf (stderr, "Bad input file syntax - expected <filepath>:<stage>\n");
			std::exit (1);
		}

		EShLanguage stage;

		if (stageStr == "vert")
			stage = EShLanguage::EShLangVertex;
		else if (stageStr == "frag")
			stage = EShLanguage::EShLangFragment;
		else {
			std::fprintf (stderr, "Unknown shader stage '%s'\n", stageStr.c_str ());
			std::exit (1);
		}

		const InputShaderFile shaderFile(filepath, stage);
		sShaderFiles.push_back(shaderFile);
	}
}
