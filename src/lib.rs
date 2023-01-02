use proc_macro::TokenStream;

#[proc_macro]
pub fn replace(items: TokenStream) -> TokenStream {
	let mut items_str = items.to_string();

	while let (Some(start), Some(end)) = (items_str.find("[<"), items_str.find(">]")) {
		let mut replace = items_str[start + 2..end].to_string();

		while let Some(upper_start) = replace.find(" : upper") {
			let before_upper = &replace[..upper_start];
			let word_start = before_upper.rfind(|c: char| c.is_whitespace())
				.map(|i| i + 1).unwrap_or(0);
			let word = &before_upper[word_start..];
			let upper_word = word.to_uppercase();

			replace.replace_range(word_start..upper_start + 8, &upper_word);
		}

		replace.retain(|c| !c.is_whitespace());

		items_str.replace_range(start..end + 2, &replace);
	}

	items_str.parse().unwrap()
}