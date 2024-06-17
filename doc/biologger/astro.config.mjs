import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

// https://astro.build/config
export default defineConfig({
    site: 'https://markovejnovic.github.io/biologger-fw/',
    base: '/biologger-fw',
	integrations: [
		starlight({
			title: 'FLoggy',
			social: {
				github: 'https://github.com/markovejnovic/biologger',
			},
			sidebar: [
				{
					label: 'Usage',
					autogenerate: { directory: 'usage' },
				},
				{
					label: 'Firmware',
					autogenerate: { directory: 'firmware' },
				},
			],
		}),
	],
});
