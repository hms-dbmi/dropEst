#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE tests

#include <boost/test/unit_test.hpp>

#include "TagsSearch/FixPosSpacerTagsFinder.h"
#include "TagsSearch/SpacerFinder.h"
#include "TagsSearch/SpacerTagsFinder.h"
#include "TagsSearch/TagsFinderBase.h"
#include "Tools/Logs.h"
#include "Tools/ReadParameters.h"

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace TagsSearch;

struct Fixture
{
	Fixture()
	{
		Tools::init_test_logs(); //boost::log::trivial::debug
		std::stringstream config;
		config << "<config>\n"
				"    <SpacerSearch>\n"
				"        <spacer>GAGTGATTGCTTGTGACGCCTT</spacer>\n"
				"        <max_spacer_edit_distance>3</max_spacer_edit_distance>\n"
				"        <spacer_prefix_length>5</spacer_prefix_length>\n"
				"        <spacer_min_pos>8</spacer_min_pos>\n"
				"        <spacer_max_pos>11</spacer_max_pos>\n"
				"        <barcode_length>8</barcode_length>\n"
				"        <umi_length>6</umi_length>\n"
				"        <r1_rc_length>8</r1_rc_length>\n"
				"    </SpacerSearch>\n"
				"    <TailTrimming>\n"
				"        <min_align_length>10</min_align_length>\n"
				"        <max_reads>10000000</max_reads>\n"
				"        <poly_a_tail>AAAAAAAA</poly_a_tail>\n"
				"    </TailTrimming>\n"
				"</config>";

		boost::property_tree::ptree pt;
		read_xml(config, pt);

		config.clear();
		config << "    <SpacerSearch>\n"
				"        <barcode_mask>[20]TGAC[20]TCCC[20]CAACGAGGTCGGCTAGGCG(8)</barcode_mask>\n"
				"        <spacer_edit_dists>2,2,7</spacer_edit_dists>\n"
				"        <r1_rc_length>8</r1_rc_length>\n"
				"    </SpacerSearch>\n";

		boost::property_tree::ptree pt2;
		read_xml(config, pt2);

		this->spacer_finder = SpacerFinder(pt.get_child("config.SpacerSearch"));
		this->tags_finder = std::make_shared<SpacerTagsFinder>(nullptr, pt.get_child("config.SpacerSearch"), pt.get_child("config.TailTrimming"));
		this->mask_tags_finder = std::make_shared<FixPosSpacerTagsFinder>(nullptr, pt2.get_child("SpacerSearch"), pt.get_child("config.TailTrimming"));
	}

	SpacerFinder spacer_finder;
	std::shared_ptr<SpacerTagsFinder> tags_finder;
	std::shared_ptr<FixPosSpacerTagsFinder> mask_tags_finder;
};

BOOST_AUTO_TEST_SUITE(TestTagsSearch)

	BOOST_FIXTURE_TEST_CASE(test1, Fixture)
	{
		std::string r1_line2 = "TTCGGTTCGGAGTGATTGCTTGTGACGCCTTCTTCGATTCGCCATTTTTTTTTTT";
		std::string r2_line2 = "TTGTTTCGCCCGGTTTTCTGTTTTCAGTAAAGTCTCGTTACGCCAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
		std::string r2_line3 = "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++";

		auto spacer_pos = tags_finder->spacer_finder.find_spacer(r1_line2);
		std::string barcodes_tail = this->spacer_finder.parse_r1_rc(r1_line2, spacer_pos.second);
		tags_finder->trim(barcodes_tail, r2_line2, r2_line3);

		BOOST_CHECK_EQUAL(spacer_pos.first, 9);
		BOOST_CHECK_EQUAL(spacer_pos.second, 31);
		BOOST_CHECK_EQUAL(r2_line2.size(), 44);
		BOOST_CHECK_EQUAL(r2_line2.size(), r2_line3.size());
	}

	BOOST_FIXTURE_TEST_CASE(test2, Fixture)
	{
		std::string r1_seq = "TAGTTTCGGAGTGTTTGCTTGTGACGCCTTACCTTGCCCGCGACTTTTTTTTTTT";
		std::string r2_seq = "TCTTCCACTAATAGTTATGTCATCCCTCTTATTAATCATCATCCTAGCCCTAAGTCTGGCCTATGAGTCACTACAAAAAGGATTAGACTGAACCG";
		std::string r2_quality_str = "1>111@1@111@33AA3BAA33DE1AA0FF3DA33AB3AF3D2A12110AB000DFGD01F10A121A11A2BFB110/AA0ABG111A111BF>";
		Tools::ReadParameters res = tags_finder->parse_and_trim(r1_seq, "r2_id", r2_seq, r2_quality_str);
		BOOST_CHECK_EQUAL(res.is_empty(), false);
		BOOST_CHECK_EQUAL(res.cell_barcode(), "TAGTTTCGACCTTGCC");
	}

	BOOST_FIXTURE_TEST_CASE(test3, Fixture)
	{
		std::string r1_seq = "TGACCATTACTGAGTGATTGCTTGTGACGCCTTAAGCGTACAGATTATTTT";
		std::string r2_seq = "GACTGGTTGAAATTGATGATTGACATTAATAATGA";
		std::string r2_quality_str = "GACTGGTTGAAATTGATGATTGACATTAATAATGA";
		Tools::ReadParameters res = tags_finder->parse_and_trim(r1_seq, "r2_id", r2_seq, r2_quality_str);
		BOOST_CHECK_EQUAL(res.is_empty(), false);
	}

	BOOST_FIXTURE_TEST_CASE(testMask, Fixture)
	{
		typedef FixPosSpacerTagsFinder::MaskPart::Type mask_part_t;
		auto const & mask_parts = this->mask_tags_finder->_mask_parts;
		BOOST_REQUIRE_EQUAL(mask_parts.size(), 7);

		mask_part_t types[] = {mask_part_t::CB, mask_part_t::SPACER, mask_part_t::CB, mask_part_t::SPACER,
							   mask_part_t::CB, mask_part_t::SPACER, mask_part_t::UMI};
		std::string spacers[] = {"", "TGAC", "", "TCCC", "", "CAACGAGGTCGGCTAGGCG", ""};
		size_t lengths[] = {20, 4, 20, 4, 20, 19, 8};

		for (int i = 0; i < 7; ++i)
		{
			BOOST_CHECK_EQUAL(mask_parts[i].type, types[i]);
			BOOST_CHECK_EQUAL(mask_parts[i].spacer, spacers[i]);
			BOOST_CHECK_EQUAL(mask_parts[i].length, lengths[i]);
		}
	}

	BOOST_FIXTURE_TEST_CASE(testMaskParse, Fixture)
	{
		std::string seq("TCTCACTGCGTCTCACTGCGTGACATTGTCGGCCATTGTCGGCCTCCCGGAGATAGGAGGAGATAGGACAACGAGGTCGGCTAGGCGTAAGGGATTTTTTTTTTTTTTTTT");
		Tools::ReadParameters params;
		this->mask_tags_finder->parse(seq, "TestId", params);
		BOOST_CHECK_EQUAL(params.read_name(), "TestId");
		BOOST_CHECK_EQUAL(params.cell_barcode(), "TCTCACTGCGTCTCACTGCGATTGTCGGCCATTGTCGGCCGGAGATAGGAGGAGATAGGA");
		BOOST_CHECK_EQUAL(params.umi_barcode(), "TAAGGGAT");
	}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TestSpacerFinder)

	BOOST_FIXTURE_TEST_CASE(testTotal, Fixture)
	{
		std::string r1_line2 = "TAGTCTAGGAGTGATTGCTTGTGACGCCTTTCATCCTTATAATATTTTTTTTTTT";
		auto spacer_pos = spacer_finder.find_spacer(r1_line2);
		BOOST_CHECK_EQUAL(spacer_pos.first, 8);
		BOOST_CHECK_EQUAL(spacer_pos.second, 30);
		BOOST_CHECK_EQUAL(spacer_finder.parse_cell_barcode(r1_line2, spacer_pos.first, spacer_pos.second), "TAGTCTAGTCATCCTT");
		BOOST_CHECK_EQUAL(spacer_finder.parse_umi_barcode(r1_line2, spacer_pos.second), "ATAATA");
	}

	BOOST_FIXTURE_TEST_CASE(testSuffix, Fixture)
	{
		std::string r1_line2 = "TAGTTTCGGAGTGTTTGCTTGTGACGCCTTACCTTGCCCGCGACTTTTTTTTTTT";
		auto spacer_pos = spacer_finder.find_spacer(r1_line2);
		BOOST_CHECK_EQUAL(spacer_pos.first, 8);
		BOOST_CHECK_EQUAL(spacer_pos.second, 30);
	}

	BOOST_FIXTURE_TEST_CASE(testPrefix, Fixture)
	{
		std::string r1_line2 = "TAGTCTAGGAGTGATTGCTTGTGACGGGTTTCATCCTTATAATATTTTTTTTTTT";
		auto spacer_pos = spacer_finder.find_spacer(r1_line2);
		BOOST_CHECK_EQUAL(spacer_pos.first, 8);
		BOOST_CHECK_EQUAL(spacer_pos.second, 30);
	}


BOOST_AUTO_TEST_SUITE_END()
