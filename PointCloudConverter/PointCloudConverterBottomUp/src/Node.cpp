#include "Node.h"

std::string Node::BasePath("./");

void Node::flush()
{
    // TODO
    // ‚±‚Ì‚ ‚Æ‚Émerge‚³‚ê‚é‚ÆAdepth‚ª•Ï‚í‚Á‚Ä‚µ‚Ü‚¤.
    // flush ‚ª‚P“x‚Å‚àŒÄ‚Î‚ê‚½‚çmerge‚³‚ê‚È‚¢‚æ‚¤‚É‚·‚éH.

    if (!m_fp) {
        std::string path(BasePath);

        auto depth = getDepth();
        auto mortonNumber = getMortonNumber();

        path += "r";

        for (uint32_t i = 1; i < depth; i++) {
           // TODO 
        }

        fopen_s(&m_fp, path.c_str(), "wb");
        IZ_ASSERT(m_fp);
    }

    // TODO
}

void Node::close()
{
    IZ_ASSERT(m_fp);

    // TODO

    fclose(m_fp);
}
