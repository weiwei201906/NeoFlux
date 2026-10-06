<script setup lang="ts">
import Giscus from '@giscus/vue'
import { useData } from 'vitepress'
import DefaultTheme from 'vitepress/theme'

const { isDark, lang } = useData()
</script>

<template>
  <DefaultTheme.Layout>
    <template #doc-after>
      <section class="giscus-comments" aria-labelledby="giscus-heading">
        <h2 id="giscus-heading">
          {{ lang.startsWith('zh') ? '讨论' : 'Comments' }}
        </h2>
        <ClientOnly>
          <Giscus
            repo="weiwei201906/NeoFlux"
            repo-id="R_kgDOT52tkA"
            category="Q&A"
            category-id="DIC_kwDOT52tkM4DG4Cb"
            mapping="pathname"
            strict="0"
            reactions-enabled="1"
            emit-metadata="1"
            input-position="top"
            :theme="isDark ? 'dark' : 'light'"
            :lang="lang.startsWith('zh') ? 'zh-CN' : 'en'"
            loading="lazy"
          />
        </ClientOnly>
      </section>
    </template>
  </DefaultTheme.Layout>
</template>

<style scoped>
.giscus-comments {
  margin-top: 3rem;
  padding-top: 1.5rem;
  border-top: 1px solid var(--vp-c-divider);
}

.giscus-comments h2 {
  margin: 0 0 1rem;
  font-size: 1.25rem;
  font-weight: 600;
}
</style>