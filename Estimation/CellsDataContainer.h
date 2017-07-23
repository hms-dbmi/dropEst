#pragma once

#include "Stats.h"
#include "Tools/IndexedValue.h"

#include <string>

#include <map>
#include <vector>
#include <unordered_map>
#include <Tools/GtfRecord.h>

namespace TestEstimator
{
	struct testMerge;
	struct testGeneMatchLevelUmiExclusion;
}

namespace Estimation
{
	namespace Merge
	{
		class MergeStrategyAbstract;
		namespace UMIs
		{
			class MergeUMIsStrategySimple;
		}
	}

	class CellsDataContainer
	{
		friend struct TestEstimator::testMerge;
		friend struct TestEstimator::testGeneMatchLevelUmiExclusion;

	public:
		class Mark
		{
		public:
			enum MarkType
			{
				NONE = 0,
				HAS_NOT_ANNOTATED = 1,
				HAS_EXONS = 2,
				HAS_INTRONS = 4
			};
		private:
			char _mark;
		public:
			static const std::string DEFAULT_CODE;

			Mark(MarkType type = MarkType::NONE);

			void add(const Mark &mark);
			void add(MarkType type);
			void add(Tools::GtfRecord::RecordType type);
			bool check(MarkType type) const;
			bool match(const std::vector<Mark>) const;
			bool operator==(const MarkType &other) const;
			bool operator==(const Mark &other) const;

			static Mark get_by_code(char code);
			static std::vector<Mark> get_by_code(const std::string &code);
		};

		class UMI
		{
		public:
			size_t read_count;
			Mark mark;

		public:
			UMI(size_t read_count = 0);
			void merge(const UMI& umi);
		};

		typedef std::unordered_map<std::string, size_t> s_ul_hash_t;
		typedef std::unordered_map<std::string, s_ul_hash_t> ss_ul_hash_t;
		typedef std::unordered_map<std::string, std::string> s_s_hash_t;

		typedef std::map<std::string, UMI> umi_map_t;
		typedef std::map<std::string, umi_map_t> genes_t;

		typedef std::vector<Tools::IndexedValue> i_counter_t;
		typedef std::vector<size_t> ids_t;
		typedef std::vector<size_t> counts_t;
		typedef std::vector<std::string> names_t;
		typedef std::vector<bool> flags_t;

	private:
		std::shared_ptr<Merge::MergeStrategyAbstract> _merge_strategy;
		std::shared_ptr<Merge::UMIs::MergeUMIsStrategySimple> _umi_merge_strategy;

		static const size_t TOP_PRINT_SIZE;
		const int _max_cells_num;

		std::vector<genes_t> _cells_genes; //cell_id -> gen_name -> umi -> count
		names_t _cell_barcodes;
		flags_t _is_cell_excluded;
		flags_t _is_cell_merged;
		s_ul_hash_t _cell_ids_by_cb;
		ids_t _filtered_cells;
		ids_t _merge_targets;

		counts_t _cell_sizes;
		i_counter_t _filtered_cells_gene_counts_sorted;
		bool _is_initialized;
		const std::vector<Mark> _gene_match_levels;

		size_t _has_exon_reads;
		size_t _has_intron_reads;
		size_t _has_not_annotated_reads;
		size_t _number_of_real_cells;

		mutable Stats _stats;

	private:
		std::string get_cb_count_top_verbose(const i_counter_t &cells_genes_counts) const;

	public:
		CellsDataContainer(std::shared_ptr<Merge::MergeStrategyAbstract> merge_strategy,
		                   std::shared_ptr<Merge::UMIs::MergeUMIsStrategySimple> umi_merge_strategy,
		                   const std::vector<Mark> &gene_match_levels, int max_cells_num = -1);

		size_t add_record(const std::string &cell_barcode, const std::string &umi, const std::string &gene,
		                  const Mark &umi_mark = Mark::HAS_EXONS);
		void exclude_cell(size_t index);

		void merge_and_filter();
		void merge_cells(size_t source_cell_ind, size_t target_cell_ind);

		void merge_umis(size_t cell_id, const std::string &gene, const s_s_hash_t &merge_targets);

		void update_cell_sizes(int genes_threshold, int cell_threshold, bool logs);

		void set_initialized();

		const names_t& cell_barcodes_raw() const;
		const i_counter_t& cells_gene_counts_sorted() const;
		const s_ul_hash_t& cell_ids_by_cb() const;
		names_t excluded_cells() const;
		const ids_t& filtered_cells() const;
		const ids_t& merge_targets() const;
		const std::vector<CellsDataContainer::Mark>& gene_match_level() const;

		const std::string &cell_barcode(size_t index) const;
		const genes_t &cell_genes(size_t index) const;
		size_t cell_size(size_t cell_index) const;
		bool is_cell_excluded(size_t cell_id) const;
		bool is_cell_merged(size_t cell_id) const;
		bool is_cell_real(size_t cell_id) const;

		s_ul_hash_t query_umis_per_gene(size_t cell_id, bool return_reads) const;
		ss_ul_hash_t query_read_per_umi_per_gene(size_t cell_id) const;

		size_t has_exon_reads_num() const;
		size_t has_intron_reads_num() const;
		size_t has_not_annotated_reads_num() const;
		size_t number_of_real_cells() const;

		Stats &stats() const;
		std::string merge_type() const;

		s_ul_hash_t umis_distribution() const;
	};
}