<script lang="ts">
	import { serverStore } from '$lib/stores';

	interface Props {
		isEmpty: boolean;
	}

	let { isEmpty = false }: Props = $props();
</script>

<div
	class={[
		'pointer-events-none mb-4 hidden px-4 text-center text-balance',
		isEmpty && 'mb-[calc(50dvh-8rem)] md:mb-8 pointer-events-auto block!'
	]}
>
	<h1 class="mb-2 text-2xl font-semibold tracking-tight md:text-3xl">小瀚能帮你做些什么？</h1>

	<p class="text-muted-foreground md:text-lg">
		{serverStore.props?.modalities?.audio ? '录制音频、输入想法' : '输入想法'}或上传文件开始使用
	</p>
</div>
